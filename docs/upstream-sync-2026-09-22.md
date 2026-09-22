# Valhalla: мерж upstream у tools, 22 верасня 2026

Гісторыі `github/tools` і `gitlab/tools` зведзеныя на поўным каміце `2fae8397b`
(`Fix elevation utility tile handling`). GitLab-версія `347bf10c2` з тым жа бацькам
не змяшчала папраўку CLI. Шэсць наступных камітаў GitLab перанесеныя паверх
`2fae8397b`; `git range-diff` пацвярджае захаванне ўсіх шасці патчаў без зменаў.

Новая база нашай галіны: `eb8500ec4`. Паверх яе зроблены адзін мерж афіцыйнага
`upstream/master`: `e8b40078ef7f983ad0aded486f84f63aae69d330` (пасля рэлізу 3.9.0).
Прапановы ніжэй — лакальны план; PR ці паведамленняў у upstream не адпраўлена.

## Што прыбрана

- Наш `565fc3673` у `meili/match_route.cc`: адпаведнае выпраўленне ўжо ёсць у
  upstream (`9eaaf5d3a`, PR #6278), разам з тэстам паўторных GPS-кропак.
- Стары `cc39f42e0` з ачысткай `TimeDomain`: ён губляў дзень тыдня і месяцы для
  другога інтэрвалу. Прыняты цалкам новы парсер upstream (`979a443d2`, PR #6230).
- Невыкарыстоўваны `GraphReader::getSourceForRT`, яго `changed()` і `_mtime`.
  Чытанне нашых RT-архіваў праз callbacks захавана.
- Пераход на `fmt` у 16 файлах тэстаў: у гэтых файлах не было іншых нашых зменаў.
  Яны цяпер дакладна адпавядаюць upstream. Гэты код не ўваходзіць у мабільную бібліятэку.
- Выдаленыя upstream генератар `locales/make_locales.sh` і стары route CLI.
  Мабільная зборка пераведзена на наяўны CMake-генератар лакаляў; спісы крыніц
  абноўленыя пасля выдалення `location.cc`, `pathlocation.cc` і перайменавання multimodal.

## Што захавана і адаптавана

- Нашы `micro`, RT/mmap, паўторнае выкарыстанне extract/elevation-кэшаў,
  мабільны fallback для PMR і static initialization, `fmt` у runtime-кодзе.
- Захаванне `DirectedEdgeExt`, прасторавых bins і predicted speeds пры перазапісе
  тайлаў. Перанесена на upstream `tile_ostream`, які вылічае кантрольную суму.
  Цяпер захоўваюцца таксама новыя bounding circles і deduplicated speed profiles.
- Hiking ETA: выпраўлена інтэрпаляцыя ў зваротным кірунку. Апошні інтэрвал
  выбарак вышыні можа быць карацейшы за 60 м, таму нельга проста перавярнуць
  масіў вышынь. Зваротны разлік цяпер выкарыстоўвае зыходныя пазіцыі выбарак.
- З upstream захаваныя абмежаванне grade для мастоў/тунэляў, новыя pedestrian
  use factors, incident layers, tile hashes і абнаўленне build ID набора тайлаў.
- Выпраўлена канфігурацыя тэстаў пры `ENABLE_PYTHON_BINDINGS=OFF`: Python interpreter
  знаходзіцца асобна, залежнасць ад `run-scripts` дадаецца толькі калі target існуе.
- Dockerfile атрымаў новую абавязковую залежнасць `spatialite-bin` і этап `dependencies`.
- Поўная папраўка CLI `valhalla_add_elevation` з `2fae8397b`: апрацоўка аргументаў
  і шляхоў да тайлаў захаваная з новым API `GraphId::FromTilePath`.

## Што прапанаваць upstream

| Прыярытэт | Змена | Карысць і неабходная падрыхтоўка |
| --- | --- | --- |
| 1 | Захаванне даных пры абнаўленні тайлаў (`de3fc51f6`, `1dce60f1f`, `1aa8a0fbc`) | Агульнае выпраўленне страты/зруху даных. Асобныя невялікія PR: extension records; deserialize/store round trip для bins, bounding circles і traffic. Рэгрэсійны тэст ужо правярае Update, UpdatePredictedSpeeds і два StoreTileData з ростам секцыі імёнаў. Пры пераносе ў чысты upstream тэст extensions трэба зрабіць незалежным ад нашага hiking API. |
| 2 | CLI `valhalla_add_elevation` (`2fae8397b`) | Утыліта чытала спіс тайлаў з `concurrency` і заўсёды выходзіла з памылкай. Папраўка аргументаў і шляхоў карысная незалежна ад нашых hiking-зменаў. Дададзеныя пяць CLI-тэстаў з сапраўдным тайлам і сінтэтычнымі вышынямі; яны ўзнаўляюць збой да папраўкі. |
| 3 | `memcpy` для `ConditionalSpeedLimit` (`f4dccee77`) | Пазбягае невыраўнаванага доступу з SIGBUS на armv7 і undefined behavior на іншых платформах. Малы агульны fix; перад PR патрэбны мэтанакіраваны тэст з alignment sanitizer або на armv7. |
| 4 | Тэсты без Python bindings (знойдзена падчас гэтага мержу) | Простая памылка CMake; Docker-канфігурацыя з tests ON, bindings OFF яе ўзнаўляла. Выпраўленне не патрабуе нашай мабільнай інфраструктуры. |
| 5 | Аднолькавы leg_shape_index для блізкіх through-кропак (`48abdbb51`) | Патэнцыйна карыснае выпраўленне памылкі 500/499. Перад PR патрэбны асобны Gurka-тэст з некалькімі through-кропкамі на адным рабры; агульных тэстаў route недастаткова для доказу гэтага выпадку. |

Не адпраўляць адным вялікім PR:

- **Hiking ETA.** Спачатку абмеркаваць мадэль часу, крыніцу крывой, адсутныя DEM-даныя,
  масты/тунэлі і захоўванне ў тайлах. Цяперашняя рэалізацыя дадае 8 байтаў
  `DirectedEdgeExt` на кожнае накіраванае рабро ў ўзбагачаным тайле, нават калі
  hiking time патрэбны толькі частцы рэбраў. Гэта не 8 байтаў толькі на пешаходнае рабро.
  Патрэбныя вымярэнні памеру набора тайлаў і параўнанне ETA з рэальнымі трэкамі.
- **Observed timestamps у Meili (`eb8500ec4`).** Пакінуць у сябе, але перад upstream
  параўнаць абедзве мадэлі на стаянках, GPS-шуме, павольным руху і петлях.
  Павелічэнне дапушчальнага часу можа як вярнуць правільны шлях, так і дазволіць лішні.
- **Агульныя кэшы.** Прапанаваць толькі пасля вымярэння cold/warm startup і RSS,
  з выразным кантрактам абнаўлення файлаў пад тым жа шляхам.
- **PMR / ConstFlatMap / fmt.** Мабільныя абыходы пакуль патрэбныя нам;
  для upstream вылучаць мінімальны ўзнаўляльны збой канкрэтнага toolchain.
- **RT-фармат, micro wrapper, tracer, GitLab publication.** Пакуль застаюцца
  інтэграцыяй Guru Maps, без падстаў прапаноўваць увесь гэты пласт upstream.

## Праверка

Вобраз: Ubuntu 24.04 arm64, Clang 18, этап `dependencies` нашага Dockerfile.
Канфігурацыя: RelWithDebInfo, tests ON, Python bindings OFF.
Тэставыя даныя ў Linux volume; на агульным каталогу macOS былі два SIGBUS падчас
пабудовы тайлаў. На Linux volume map-matching suite прайшоў двойчы запар.
Гэта лакалізуе праблему асяроддзя, але не даказвае канкрэтную прычыну SIGBUS.

Кампіляцыя з `VALHALLA_MOBILE=1` праверана для costmatrix, sample, graphreader,
graphtile, tile_action і micro.cpp. Гэта праверка галін кода ў Linux,
не поўная зборка iOS/Android SDK і не праверка armv7.

Пасля звядзення гісторый і падрыхтоўкі адзінага мержу ўсе праверкі паўтораныя.
Поўная звычайная зборка бібліятэкі, сэрвіса, CLI-ўтыліт і tracer паспяховая.
Усе 411 C++ тэстаў з 25 выбраных набораў і 5 CLI-тэстаў прайшлі (разам 416).
Правераныя timeparsing, directededge,
graphtilebuilder, graphreader, edgeinfo, costing, elevation, trace attributes, vector tiles,
route, matrix, avoids, barrier U-turns, closures, traffic, HOV, roundabout instructions,
linear feature factors, OSRM serialization, search filters, side of street і hierarchy limits.
Поўны набор усіх тэстаў праекта і асобная зборка production Docker image не запускаліся.
Праверкі сінтаксісу Xcode project і шасці мабільных адзінак трансляцыі таксама паспяховыя.
Лагі паўторнага прагону — `build-merge-docker/history-*.log`, зводка —
`build-merge-docker/history-results.json` (каталог зборкі не каміціцца).
