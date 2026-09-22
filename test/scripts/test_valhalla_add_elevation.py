import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


class TestAddElevation(unittest.TestCase):
    def setUp(self):
        self.binary = Path('valhalla_add_elevation').resolve()
        self.tempdir = tempfile.TemporaryDirectory(prefix='valhalla-add-elevation-')
        self.addCleanup(self.tempdir.cleanup)
        self.tile_dir = Path(self.tempdir.name) / 'tiles'
        self.relative_tile = Path('2/000/818/660.gph')
        self.tile = self.tile_dir / self.relative_tile
        self.tile.parent.mkdir(parents=True)
        self.source_tile = Path('test/data/utrecht_tiles') / self.relative_tile
        shutil.copyfile(self.source_tile, self.tile)
        elevation_dir = Path(self.tempdir.name) / 'elevation'
        elevation_dir.mkdir()
        (elevation_dir / 'N52E005.hgt').write_bytes(b'\x00\x64' * (3601 * 3601))
        self.config = json.dumps({
            'mjolnir': {'tile_dir': str(self.tile_dir), 'concurrency': 1},
            'additional_data': {'elevation': str(elevation_dir)},
        })

    def run_tool(self, *args):
        return subprocess.run(
            [str(self.binary), '--inline-config', self.config, *args],
            capture_output=True, text=True, timeout=30)

    def test_tile_paths(self):
        paths = [str(self.tile), os.path.relpath(self.tile),
                 str(self.relative_tile), '/' + str(self.relative_tile)]
        for path in paths:
            for concurrency in ([], ['--concurrency', '1']):
                with self.subTest(path=path, concurrency=concurrency):
                    shutil.copyfile(self.source_tile, self.tile)
                    before = self.tile.read_bytes()
                    result = self.run_tool('--tiles', path, *concurrency)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    self.assertNotEqual(self.tile.read_bytes(), before)

    def test_missing_tiles_option(self):
        result = self.run_tool()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Tile file is required', result.stderr)

    def test_empty_tiles(self):
        result = self.run_tool('--tiles', '')
        self.assertNotEqual(result.returncode, 0)

    def test_missing_tile_file(self):
        result = self.run_tool('--tiles', '2/000/818/661.gph')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Failed to load tiles', result.stderr)

    def test_mixed_valid_and_missing_tiles(self):
        before = self.tile.read_bytes()
        result = self.run_tool('--tiles', str(self.relative_tile),
                               '--tiles', '2/000/818/661.gph')
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertNotEqual(self.tile.read_bytes(), before)


if __name__ == '__main__':
    unittest.main()
