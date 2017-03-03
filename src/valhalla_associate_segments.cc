#include "midgard/logging.h"
#include "baldr/graphreader.h"
#include "baldr/merge.h"
#include "loki/search.h"
#include "loki/node_search.h"
#include "thor/pathalgorithm.h"
#include "thor/astar.h"
#include "mjolnir/graphtilebuilder.h"

#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/iterator/reverse_iterator.hpp>

#include <deque>
#include <algorithm>
#include <thread>
#include <mutex>
#include <future>

#include "config.h"
#include "segment.pb.h"
#include "tile.pb.h"

namespace vm = valhalla::midgard;
namespace vb = valhalla::baldr;
namespace vl = valhalla::loki;
namespace vs = valhalla::sif;
namespace vt = valhalla::thor;
namespace vj = valhalla::mjolnir;
namespace pbf = opentraffic::osmlr;

namespace bal = boost::algorithm;
namespace bpo = boost::program_options;
namespace bpt = boost::property_tree;
namespace bfs = boost::filesystem;

namespace std {
std::string to_string(const vm::PointLL &p) {
  std::ostringstream out;
  out.precision(16);
  out << "PointLL(" << p.lat() << ", " << p.lng() << ")";
  return out.str();
}

std::string to_string(const vb::GraphId &i) {
  std::ostringstream out;
  out << "GraphId(" << i.tileid() << ", " << i.level() << ", " << i.id() << ")";
  return out.str();
}
} // namespace std

namespace {

// 10 meter length matching tolerance
constexpr uint32_t kLengthTolerance = 10;

enum class MatchType : uint8_t {
  kWalk = 0,
  kShortestPath = 1
};

struct MatchStats {
  uint32_t success_count;
  uint32_t failure_count;
  uint32_t walk_count;
  uint32_t path_count;

  MatchStats()
    : success_count(0),
      failure_count(0),
      walk_count(0),
      path_count(0) {
  }
};

vm::PointLL interp(vm::PointLL a, vm::PointLL b, double frac) {
  return vm::PointLL(a.AffineCombination(1.0 - frac, frac, b));
}

// chop the first "dist" length off seg, returning it as the result. this will
// modify seg!
std::vector<vm::PointLL> chop_subsegment(std::vector<vm::PointLL> &seg, uint32_t dist) {
  const size_t len = seg.size();
  assert(len > 1);

  std::vector<vm::PointLL> result;
  result.push_back(seg[0]);
  double d = 0.0;
  size_t i = 1;
  for (; i < len; ++i) {
    auto segdist = seg[i-1].Distance(seg[i]);
    if ((d + segdist) >= dist) {
      double frac = (dist - d) / segdist;
      auto midpoint = interp(seg[i-1], seg[i], frac);
      result.push_back(midpoint);
      // remove used part of seg.
      seg.erase(seg.begin(), seg.begin() + (i - 1));
      seg[0] = midpoint;
      break;

    } else {
      d += segdist;
      result.push_back(seg[i]);
    }
  }

  // used all of seg, and exited the loop by iteration rather than breaking out.
  if (i == len) {
    seg.clear();
  }

  return result;
}

uint16_t bearing(const std::vector<vm::PointLL> &shape) {
  // OpenLR says to use 20m along the edge, but we could use the
  // GetOffsetForHeading function, which adapts it to the road class.
  float heading = vm::PointLL::HeadingAlongPolyline(shape, 20);
  assert(heading >= 0.0);
  assert(heading < 360.0);
  return uint16_t(std::round(heading));
}

uint16_t bearing(const vb::GraphTile *tile, vb::GraphId edge_id, float dist) {
  std::vector<vm::PointLL> shape;
  const auto *edge = tile->directededge(edge_id);
  uint32_t edgeinfo_offset = edge->edgeinfo_offset();
  auto edgeinfo = tile->edgeinfo(edgeinfo_offset);
  uint32_t edge_len = edge->length();

  shape = edgeinfo.shape();
  if (!edge->forward()) {
    std::reverse(shape.begin(), shape.end());
  }

  if (dist > 0.0) {
    chop_subsegment(shape, uint32_t(dist * edge_len));
  }

  return bearing(shape);
}

struct partial_chunk {
  std::vector<vb::GraphId> edges, segments;
};

using leftovers_t = std::vector<std::pair<GraphId, vb::TrafficChunk > >;
struct edge_association {
  explicit edge_association(const bpt::ptree &pt);

  MatchStats add_tile(const std::string &file_name);
  const leftovers_t& leftovers() const { return m_leftover_associations; }

private:
  bool match_segment(vb::GraphId segment_id, const pbf::Segment &segment,
                     MatchType& match_type);
  std::vector<vb::GraphId> match_edges(const pbf::Segment &segment, uint8_t level,
                                       MatchType& match_type);
  vm::PointLL lookup_end_coord(const vb::GraphId& edge_id);
  vm::PointLL lookup_start_coord(const vb::GraphId& edge_id);

  void assign_one_to_one(const vb::GraphId& edge_id, const vb::GraphId& segment_id);
  void assign_one_to_many(const std::vector<vb::GraphId> &edges, const vb::GraphId& segment_id);
  void save_chunk_for_later(const std::vector<vb::GraphId> &edges, vb::GraphId segment_id);

  vb::GraphReader m_reader;
  vs::TravelMode m_travel_mode;
  std::shared_ptr<vt::PathAlgorithm> m_path_algo;
  std::shared_ptr<vs::DynamicCost> m_costing;
  std::shared_ptr<vj::GraphTileBuilder> m_tile_builder;
  const vb::GraphTile* m_tile;
  // chunks saved for later
  std::list<partial_chunk> m_partial_chunks;
  // simple associations saved for later
  leftovers_t m_leftover_associations;
};

struct edge_score {
  vb::GraphId id;
  int score;
};

// Use this method to determine whether an edge should be allowed along the
// merged path. This method should match the predicate used to create OSMLR
// segments.
bool allow_edge_pred(const vb::DirectedEdge *edge) {
  return (!edge->trans_up() && !edge->trans_down() && !edge->is_shortcut() &&
           edge->use() != vb::Use::kTransitConnection &&
           edge->use() != vb::Use::kTurnChannel && !edge->internal() &&
           edge->use() != vb::Use::kFerry && !edge->roundabout() &&
          (edge->forwardaccess() & vb::kVehicularAccess) != 0);
}

// Check if oneway. Assumes forward access is allowed. Edge is oneway if
// no reverse vehicular access is allowed
bool is_oneway(const vb::DirectedEdge *e) {
  return (e->reverseaccess() & vb::kVehicularAccess) == 0;
}

enum class FormOfWay {
  kUndefined = 0,
  kMotorway = 1,
  kMultipleCarriageway = 2,
  kSingleCarriageway = 3,
  kRoundabout = 4,
  kTrafficSquare = 5,
  kSlipRoad = 6,
  kOther = 7
};

std::ostream &operator<<(std::ostream &out, FormOfWay fow) {
  switch (fow) {
  case FormOfWay::kUndefined:           out << "undefined";            break;
  case FormOfWay::kMotorway:            out << "motorway";             break;
  case FormOfWay::kMultipleCarriageway: out << "multiple_carriageway"; break;
  case FormOfWay::kSingleCarriageway:   out << "single_carriageway";   break;
  case FormOfWay::kRoundabout:          out << "roundabout";           break;
  case FormOfWay::kTrafficSquare:       out << "traffic_square";       break;
  case FormOfWay::kSlipRoad:            out << "sliproad";             break;
  default:
    out << "other";
  }
  return out;
}

FormOfWay form_of_way(const vb::DirectedEdge *e) {
  bool oneway = is_oneway(e);
  auto rclass = e->classification();

  // if it's a slip road, return that. TODO: am i doing this right?
  if (e->link()) {
    return FormOfWay::kSlipRoad;
  }
  // if it's a roundabout, return that
  else if (e->roundabout()) {
    return FormOfWay::kRoundabout;
  }
  // if it's a motorway and it's one-way, then it's likely to be grade separated
  else if (rclass == vb::RoadClass::kMotorway && oneway) {
    return FormOfWay::kMotorway;
  }
  // if it's a major road, and it's one-way then it might be a multiple
  // carriageway road.
  else if (rclass <= vb::RoadClass::kTertiary && oneway) {
    return FormOfWay::kMultipleCarriageway;
  }
  // not one-way, so perhaps it's a single carriageway
  else if (rclass <= vb::RoadClass::kTertiary) {
    return FormOfWay::kSingleCarriageway;
  }
  // everything else
  else {
    return FormOfWay::kOther;
  }
}

class DistanceOnlyCost : public vs::DynamicCost {
public:
  DistanceOnlyCost(vs::TravelMode travel_mode);
  virtual ~DistanceOnlyCost();
  uint32_t access_mode() const;
  bool Allowed(const vb::DirectedEdge* edge,
               const vs::EdgeLabel& pred,
               const vb::GraphTile*& tile,
               const vb::GraphId& edgeid) const;
  bool AllowedReverse(const vb::DirectedEdge* edge,
                      const vs::EdgeLabel& pred,
                      const vb::DirectedEdge* opp_edge,
                      const vb::GraphTile*& tile,
                      const vb::GraphId& edgeid) const;
  bool Allowed(const vb::NodeInfo* node) const;
  vs::Cost EdgeCost(const vb::DirectedEdge* edge) const;
  const vs::EdgeFilter GetEdgeFilter() const;
  const vs::NodeFilter GetNodeFilter() const;
  float AStarCostFactor() const;
};

DistanceOnlyCost::DistanceOnlyCost(vs::TravelMode travel_mode)
  : DynamicCost(bpt::ptree(), travel_mode) {
}

DistanceOnlyCost::~DistanceOnlyCost() {
}

uint32_t DistanceOnlyCost::access_mode() const {
  return vb::kVehicularAccess;
}

bool DistanceOnlyCost::Allowed(const vb::DirectedEdge* edge,
                               const vs::EdgeLabel& pred,
                               const vb::GraphTile*&,
                               const vb::GraphId&) const {
  // Do not allow U-turns/back-tracking
  return allow_edge_pred(edge) &&
        (pred.opp_local_idx() != edge->localedgeidx());
}

bool DistanceOnlyCost::AllowedReverse(const vb::DirectedEdge* edge,
                                      const vs::EdgeLabel& pred,
                                      const vb::DirectedEdge* opp_edge,
                                      const vb::GraphTile*& tile,
                                      const vb::GraphId& edgeid) const {
  // Do not allow U-turns/back-tracking
  return allow_edge_pred(edge) &&
        (pred.opp_local_idx() != edge->localedgeidx());
}

bool DistanceOnlyCost::Allowed(const vb::NodeInfo*) const {
  return true;
}

vs::Cost DistanceOnlyCost::EdgeCost(const vb::DirectedEdge* edge) const {
  float edge_len(edge->length());
  return {edge_len, edge_len};
}

const vs::EdgeFilter DistanceOnlyCost::GetEdgeFilter() const {
  return [](const vb::DirectedEdge *edge) -> float {
    return allow_edge_pred(edge) ? 1.0f : 0.0f;
  };
}

const vs::NodeFilter DistanceOnlyCost::GetNodeFilter() const {
  return [](const vb::NodeInfo *) -> bool {
    return false;
  };
}

float DistanceOnlyCost::AStarCostFactor() const {
  return 1.0f;
}

vm::PointLL coord_for_lrp(const pbf::Segment::LocationReference &lrp) {
  int32_t lng = lrp.coord().lng();
  int32_t lat = lrp.coord().lat();
  vm::PointLL coord(double(lng) / 10000000, double(lat) / 10000000);
  return coord;
}

vb::Location location_for_lrp(const pbf::Segment::LocationReference &lrp) {
  int32_t lng = lrp.coord().lng();
  int32_t lat = lrp.coord().lat();
  vb::Location location({double(lng) / 10000000, double(lat) / 10000000});
  if(lrp.has_bear())
    location.heading_ = lrp.bear();
  return location;
}

vb::PathLocation loki_search_single(const vb::Location &loc, vb::GraphReader &reader, uint8_t level) {
  //we dont want non real edges but also we want the edges to be on the right level
  //also right now only driveable edges please
  auto edge_filter = [level](const DirectedEdge* edge) -> float {
    return edge->endnode().level() == level && (edge->forwardaccess() & vb::kVehicularAccess) &&
      !(edge->trans_up() || edge->trans_down() || edge->is_shortcut() || edge->IsTransitLine());
  };

  //we only have one location so we only get one result
  std::vector<vb::Location> locs{loc};
  vb::PathLocation path_loc(loc);
  auto results = vl::Search(locs, reader, edge_filter, vl::PassThroughNodeFilter);
  if(results.size())
    path_loc = std::move(results.begin()->second);
  return path_loc;
}

edge_association::edge_association(const bpt::ptree &pt)
  : m_reader(pt.get_child("mjolnir"))
  , m_travel_mode(vs::TravelMode::kDrive)
  , m_path_algo(new vt::AStarPathAlgorithm())
  , m_costing(new DistanceOnlyCost(m_travel_mode))
  , m_tile(nullptr) {
}

//walk from this edge to the next one if there is only one choice of where
//to walk if there are more choices then just return invalid to signify
//stopping this is trying to mimic what osmlr generation does
vb::GraphId next_edge(const GraphId& edge_id, vb::GraphReader& reader, const vb::GraphTile*& tile,
                      uint32_t& current_length) {
  uint32_t next_length = 0;
  if(tile->id() != edge_id.Tile_Base())
    tile = reader.GetGraphTile(edge_id);
  const auto* edge = tile->directededge(edge_id);

  // Get the end node (need a new tile if the edge leaves the current tile)
  GraphId end_node = edge->endnode();
  if (edge->leaves_tile())
    tile = reader.GetGraphTile(end_node);
  const auto* node = tile->node(end_node);

  // Get child edges of this node
  const auto* child_edge = tile->directededge(node->edge_index());
  vb::GraphId next;
  for(int i = 0; i < node->edge_count(); ++i, child_edge++) {
    // Skip transition edges, shortcuts, transit connections (or transit lines)
    // and the opposing edge to the incoming edge (to prevent U-turns).
    if(i != edge->opp_index() && !child_edge->trans_up() &&
         !child_edge->trans_down() && !child_edge->is_shortcut() &&
          child_edge->use() != Use::kTransitConnection &&
         !child_edge->IsTransitLine())
    {
      // Return invalid GraphId if more than 1 candidate edge exists
      if(next.Is_Valid())
        return {};
      else {
        next = {end_node.tileid(), end_node.level(), node->edge_index() + i};
        next_length = child_edge->length();
      }
    }
  }
  current_length += next_length;
  return next;
}

struct last_tile_cache {
  last_tile_cache(GraphReader &reader) : m_reader(reader) {}

  inline const vb::GraphTile *get(vb::GraphId id) {
    if (id.Tile_Base() != m_last_id) {
      m_last_id = id.Tile_Base();
      m_last_tile = m_reader.GetGraphTile(m_last_id);
    }
    return m_last_tile;
  }

private:
  vb::GraphReader &m_reader;
  vb::GraphId m_last_id;
  const vb::GraphTile *m_last_tile;
};

// given two bearings in degrees, return the unsigned angle between them.
int bear_diff(int bear1, int bear2) {
  int bear_diff = std::abs(bear1 - bear2);
  if (bear_diff > 180) {
    bear_diff = 360 - bear_diff;
  }
  if (bear_diff < 0) {
    bear_diff += 360;
  }
  return bear_diff;
}

// given two uint32_t, return the absolute difference between them, which will
// always fit into another uint32_t.
uint32_t abs_u32_diff(uint32_t a, uint32_t b) {
  return (a > b) ? (a - b) : (b - a);
}

// Walk a path between origin and destination edges.
std::vector<vb::GraphId> walk(const vb::PathLocation &origin, const vb::PathLocation &dest,
                              vb::GraphReader& reader, const uint32_t total_length) {
  // Create a set of ending edges. Skip edges that begin at a node (outbound)
  std::unordered_set<GraphId> dest_edges;
  for (const auto &dest_edge : dest.edges) {
    if (dest_edge.begin_node()) {
      continue;
    }
    dest_edges.insert(dest_edge.id);
  }

  // Walk edges until there is no valid continuing edge (means there are no
  // valid continuing edges or more than 1) or we have exceeded the length.
  for (const auto &origin_edge : origin.edges) {
    // Skip edges ending at a node (inbound)
    if (origin_edge.end_node()) {
      continue;
    }

    // Walk a path from this edge
    auto* tile = reader.GetGraphTile(origin_edge.id);
    uint32_t walked_length = tile->directededge(origin_edge.id)->length();
    std::vector<vb::GraphId> edges{origin_edge.id};
    do {
      // Return the path edges if we found a destination edge
      if (dest_edges.find(edges.back()) != dest_edges.end() &&
          abs_u32_diff(walked_length, total_length) < kLengthTolerance) {
        return edges;
      }

      // Get the next edge, update the walked length
      edges.push_back(next_edge(edges.back(), reader, tile, walked_length));

      // Continue while next edge is valid and length does not exceed total
      // plus the tolerance
    } while(edges.back().Is_Valid() && walked_length < (total_length + kLengthTolerance));
  }
  // Edge walking did not find a candidate path
  return {};
}

std::vector<vb::GraphId> edge_association::match_edges(const pbf::Segment &segment, uint8_t level,
                                                       MatchType& match_type) {
  const size_t size = segment.lrps_size();
  assert(size >= 2);

  // calculate total length of the segment for comparison to common edges or
  // short "walked" paths.
  uint32_t total_length = 0;
  for (const auto &lrp : segment.lrps()) {
    total_length += lrp.length();
  }

  auto origin_coord = coord_for_lrp(segment.lrps(0));
  auto dest_coord = coord_for_lrp(segment.lrps(size - 1));

  auto origin = loki_search_single(vb::Location(origin_coord), m_reader, level);
  std::remove_if(origin.edges.begin(), origin.edges.end(), [](const vb::PathLocation::PathEdge& e){return e.end_node();});
  auto dest = loki_search_single(vb::Location(location_for_lrp(segment.lrps(size - 1))), m_reader, level);
  std::remove_if(dest.edges.begin(), dest.edges.end(), [](const vb::PathLocation::PathEdge& e){return e.begin_node();});

  // check if its a trivial path between edges
  auto walked_edges = walk(origin, dest, m_reader, total_length);
  if (walked_edges.size()) {
    // Check bearing, road classification, and form of way
    auto *tile = m_reader.GetGraphTile(walked_edges.front());
    auto *edge = tile->directededge(walked_edges.front());
    auto &lrp = segment.lrps(0);

    vb::RoadClass road_class = vb::RoadClass(lrp.start_frc());
    uint16_t walked_bearing = bearing(tile, walked_edges.front(), 0.0);
    int bear = bear_diff(walked_bearing, lrp.bear());
    FormOfWay fow = FormOfWay(lrp.start_fow());
    if ((road_class == edge->classification()) &&
        (bear < 10) && (fow == form_of_way(edge))) {

      // if the edge matches all our expectations, then we can just assume
      // we found the right edge and return it. this should be significantly
      // faster than running a whole route to check.
      // TODO: the first and last match edges could be partial, the TrafficAssociation
      // must be made aware of this and use the percentages returned in the PathEdge
      match_type = MatchType::kWalk;
      return walked_edges;
    } else {
     if (road_class != edge->classification()) {
        LOG_DEBUG("Road class mismatch");
      } else if (bear >= 10) {
        LOG_DEBUG("Bearing mismatch: bear = " + std::to_string(walked_bearing) + " LRP bearing = " +
                 std::to_string(lrp.bear()));
      } else {
        LOG_DEBUG("Form of way mismatch");
      }
    }
  }

  // check all the interim points of the location reference
  std::vector<vb::GraphId> edges;
  for (size_t i = 0; i < size - 1; ++i) {
    auto &lrp = segment.lrps(i);
    auto coord = coord_for_lrp(lrp);
    auto next_coord = coord_for_lrp(segment.lrps(i+1));

    vb::RoadClass road_class = vb::RoadClass(lrp.start_frc());

    dest = loki_search_single(location_for_lrp(segment.lrps(i+1)), m_reader, level);
    if (dest.edges.size() == 0) {
      LOG_DEBUG("Unable to find edge near point " + std::to_string(next_coord) + ". Segment cannot be matched, discarding.");
      return std::vector<vb::GraphId>();
    }

    // make sure there's no state left over from previous paths
    m_path_algo->Clear();
    auto path = m_path_algo->GetBestPath(
      origin, dest, m_reader, &m_costing, m_travel_mode);

    if (path.empty()) {
      // what to do if there's no path?
      LOG_DEBUG("No route to destination " + std::to_string(next_coord) + " from origin point " + std::to_string(coord) + ". Segment cannot be matched, discarding.");
      return std::vector<vb::GraphId>();
    }

    {
      auto last_edge_id = path.back().edgeid;
      auto *tile = m_reader.GetGraphTile(last_edge_id);
      auto *edge = tile->directededge(last_edge_id);
      auto node_id = edge->endnode();
      auto *ntile = (last_edge_id.Tile_Base() == node_id.Tile_Base()) ? tile : m_reader.GetGraphTile(node_id);
      auto *node = ntile->node(node_id);
      auto dist = node->latlng().Distance(next_coord);
      if (dist > 10.0f) {
        LOG_DEBUG("Route to destination " + std::to_string(next_coord) + " from origin point " + std::to_string(coord) + " ends more than 10m away: " + std::to_string(node->latlng()) + ". Segment cannot be matched, discarding.");
        return std::vector<vb::GraphId>();
      }
    }

    int score = 0;
    uint32_t sum = 0;
    for (auto &p : path) {
      sum += p.elapsed_time;
    }
    score += std::abs(int(sum) - int(lrp.length())) / 10;

    auto edge_id = path.front().edgeid;
    auto *tile = m_reader.GetGraphTile(edge_id);
    auto *edge = tile->directededge(edge_id);

    if (!allow_edge_pred(edge)) {
      LOG_DEBUG("Edge " + std::to_string(edge_id) + " not accessible. Segment cannot be matched, discarding.");
      return std::vector<vb::GraphId>();
    }
    score += std::abs(int(road_class) - int(edge->classification()));

    bool found = false;
    for (auto &e : origin.edges) {
      if (e.id == edge_id) {
        found = true;
        score += int(e.projected.Distance(coord));

        int bear1 = bearing(tile, edge_id, e.dist);
        int bear2 = lrp.bear();
        score += bear_diff(bear1, bear2) / 10;

        break;
      }
    }
    if (!found) {
      LOG_DEBUG("Unable to find edge " + std::to_string(edge_id) + " at origin point " + std::to_string(origin.latlng_) + ". Segment cannot be matched, discarding.");
      return std::vector<vb::GraphId>();
    }

    // form of way isn't really a metric space...
    FormOfWay fow1 = form_of_way(edge);
    FormOfWay fow2 = FormOfWay(lrp.start_fow());
    score += (fow1 == fow2) ? 0 : 5;

    for (const auto &info : path) {
      edges.emplace_back(info.edgeid);
    }

    // use dest as next origin
    std::swap(origin, dest);
  }

  // remove duplicate instances of the edge ID in the path info
  auto new_end = std::unique(edges.begin(), edges.end());
  edges.erase(new_end, edges.end());
  match_type = MatchType::kShortestPath;
  return edges;
}

static const float kApproxEqualDistanceSquared = 100.0f;
bool approx_equal(const vm::PointLL &a, const vm::PointLL &b) {
  return a.DistanceSquared(b) <= kApproxEqualDistanceSquared;
}

vm::PointLL edge_association::lookup_end_coord(const vb::GraphId& edge_id) {
  auto *tile = m_reader.GetGraphTile(edge_id);
  auto *edge = tile->directededge(edge_id);
  auto node_id = edge->endnode();
  auto *node_tile = tile;
  if (edge_id.Tile_Base() != node_id.Tile_Base()) {
    node_tile = m_reader.GetGraphTile(node_id);
  }
  auto *node = node_tile->node(node_id);
  return node->latlng();
}

vm::PointLL edge_association::lookup_start_coord(const vb::GraphId& edge_id) {
  auto *tile = m_reader.GetGraphTile(edge_id);
  auto *edge = tile->directededge(edge_id);
  auto opp_index = edge->opp_index();
  auto node_id = edge->endnode();
  auto *node_tile = tile;
  if (edge_id.Tile_Base() != node_id.Tile_Base()) {
    node_tile = m_reader.GetGraphTile(node_id);
  }
  auto *node = node_tile->node(node_id);
  return lookup_end_coord(node_id.Tile_Base() + uint64_t(node->edge_index() + opp_index));
}

bool edge_association::match_segment(vb::GraphId segment_id, const pbf::Segment &segment,
                                     MatchType& match_type) {
  auto edges = match_edges(segment, segment_id.level(), match_type);
  if (edges.empty()) {
    LOG_DEBUG("Unable to match segment " + std::to_string(segment_id) + ".");
    return false;
  }

  auto seg_start = coord_for_lrp(segment.lrps(0));
  auto seg_end = coord_for_lrp(segment.lrps(segment.lrps_size() - 1));

  auto edges_start = lookup_start_coord(edges.front());
  auto edges_end = lookup_end_coord(edges.back());

  if (approx_equal(seg_start, edges_start) &&
      approx_equal(seg_end, edges_end)) {
    if (edges.size() == 1) {
      // if the segment matches to one edge exactly, then we can use it
      // directly. if not then it requires a level of indirection via
      // "chunks".
      assign_one_to_one(edges.front(), segment_id);

    } else {
      // more than one edge, but matches the segment exactly. this is a
      // "one to many" case, and can also be looked up directly.
      assign_one_to_many(edges, segment_id);
    }
  } else {
    // save this for later, when we'll gather up all partial segments
    // and try to build chunks out of them.
    save_chunk_for_later(edges, segment_id);
  }
  return true;
}

// A single traffic segment maps to a single valhalla edge.
void edge_association::assign_one_to_one(const vb::GraphId& edge_id,
                                         const vb::GraphId& segment_id) {
  // Edge starts at the beginning of the traffic segment, ends on the end of
  // the traffic segment
  vb::TrafficChunk assoc(segment_id, 0.0f, 1.0f, true, true);

  // Store the local ones and leave the non local for later
  if(m_tile_builder->id() == edge_id.Tile_Base())
    m_tile_builder->AddTrafficSegment(edge_id, assoc);
  else
    m_leftover_associations.emplace_back(std::make_pair(edge_id, assoc));
}

// A single traffic segment maps to multiple valhalla edges. Each edge is
// entirely within the segment.
void edge_association::assign_one_to_many(const std::vector<vb::GraphId> &edges,
                                          const vb::GraphId& segment_id) {
  for (size_t i = 0; i < edges.size(); i++) {
    // Form association for this edge. First edge "starts" the traffic
    // segment and the last edge ends the segment.
    const auto& edge_id = edges[i];
    vb::TrafficChunk assoc(segment_id, 0.0f, 1.0f, (i == 0), (i == edges.size() - 1));

    // Store the local segment and leave the non local for later
    if(m_tile_builder->id() == edge_id.Tile_Base())
      m_tile_builder->AddTrafficSegment(edge_id, assoc);
    else
      m_leftover_associations.emplace_back(std::make_pair(edge_id, assoc));
  }
}

void edge_association::save_chunk_for_later(const std::vector<vb::GraphId> &edges, vb::GraphId segment_id) {
  partial_chunk tmp;
  tmp.edges = edges;
  tmp.segments.push_back(segment_id);
  m_partial_chunks.emplace_back(std::move(tmp));
}

vb::GraphId parse_file_name(const std::string &file_name) {
  return vb::GraphTile::GetTileId(file_name);
}

MatchStats edge_association::add_tile(const std::string &file_name) {
  //read the osmlr tile
  pbf::Tile tile;
  {
    std::ifstream in(file_name);
    if (!tile.ParseFromIstream(&in)) {
      throw std::runtime_error("Unable to parse traffic segment file.");
    }
  }

  //get a tile builder ready for this tile
  auto base_id = parse_file_name(file_name);
  m_tile_builder.reset(new vj::GraphTileBuilder(m_reader.GetTileHierarchy(), base_id, false));
  m_tile_builder->InitializeTrafficSegments();
  m_tile = m_reader.GetGraphTile(base_id);

  //do the matching of the segments in this osmlr tile
  MatchStats stats;
  std::cout.precision(16);
  uint64_t entry_id = 0;
  MatchType match_type = MatchType::kWalk;
  for (auto &entry : tile.entries()) {
    if (!entry.has_marker()) {
      assert(entry.has_segment());
      auto &segment = entry.segment();
      if (match_segment(base_id + entry_id, segment, match_type)) {
        stats.success_count++;
        if (match_type == MatchType::kWalk) {
          stats.walk_count++;
        } else {
          stats.path_count++;
        }
      } else {
        stats.failure_count++;
      }
    }
    entry_id += 1;
  }

  //finish this tile
  m_reader.Clear();
  m_tile_builder->UpdateTrafficSegments();

  return stats;
}

void add_local_associations(const bpt::ptree &pt, std::deque<std::string>& osmlr_tiles, std::mutex& lock,
  std::promise<edge_association>& association) {

  //this holds the extra data before we serialize it to the extra section
  //of a tile.
  edge_association e(pt);

  MatchStats total;
  while(true) {
    //get a file to work with
    std::string osmlr_filename;
    lock.lock();
    if(osmlr_tiles.size()) {
      osmlr_filename = std::move(osmlr_tiles.front());
      osmlr_tiles.pop_front();
    }
    lock.unlock();
    if(osmlr_filename.empty())
      break;

    //get the local associations
    auto stats = e.add_tile(osmlr_filename);

    total.success_count += stats.success_count;
    total.failure_count += stats.failure_count;
    total.walk_count += stats.walk_count;
    total.path_count += stats.path_count;
    LOG_INFO("Success = " + std::to_string(total.success_count) +
            " Failure = " + std::to_string(total.failure_count) +
            " Walk = " + std::to_string(total.walk_count) +
            " Path = " + std::to_string(total.path_count))
  }

  //pass it back
  association.set_value(std::move(e));
}

void add_leftover_associations(const bpt::ptree &pt, std::unordered_map<GraphId, leftovers_t>& leftovers,
  std::mutex& lock) {

  //something so we can open up a tile builder
  TileHierarchy hierarchy(pt.get<std::string>("mjolnir.tile_dir"));

  while(true) {
    //get a file to work with
    leftovers_t associations;
    lock.lock();
    if(leftovers.size()) {
      associations = std::move(leftovers.begin()->second);
      leftovers.erase(leftovers.begin());
    }
    lock.unlock();
    if(associations.empty())
      break;

    //write the associations
    vj::GraphTileBuilder tile_builder(hierarchy, associations.front().first.Tile_Base(), false);
    tile_builder.InitializeTrafficSegments();
    for(const auto& association : associations)
      tile_builder.AddTrafficSegment(association.first, association.second);
    tile_builder.UpdateTrafficSegments();
  }
}

} // anonymous namespace

int main(int argc, char** argv) {
  std::string config, tile_dir;
  unsigned int num_threads = 1;

  bpo::options_description options("valhalla_associate_segments " VERSION "\n"
                                   "\n"
                                   " Usage: valhalla_associate_segments [options]\n"
                                   "\n"
                                   "osmlr associates traffic segment descriptors with a valhalla graph. "
                                   "\n"
                                   "\n");

  options.add_options()
    ("help,h", "Print this help message.")
    ("version,v", "Print the version of this software.")
    ("osmlr-tile-dir,t", bpo::value<std::string>(&tile_dir), "Location of traffic segment tiles.")
    ("concurrency,j", bpo::value<unsigned int>(&num_threads), "Number of threads to use.")
    // positional arguments
    ("config", bpo::value<std::string>(&config), "Valhalla configuration file [required]");


  bpo::positional_options_description pos_options;
  pos_options.add("config", 1);
  bpo::variables_map vm;
  try {
    bpo::store(bpo::command_line_parser(argc, argv).options(options).positional(pos_options).run(), vm);
    bpo::notify(vm);
  }
  catch (std::exception &e) {
    std::cerr << "Unable to parse command line options because: " << e.what()
              << "\n" << "This is a bug, please report it at " PACKAGE_BUGREPORT
              << "\n";
    return EXIT_FAILURE;
  }

  if (vm.count("help") || !vm.count("config")) {
    std::cout << options << "\n";
    return EXIT_SUCCESS;
  }

  if (vm.count("version")) {
    std::cout << "valhalla_associate_segments " << VERSION << "\n";
    return EXIT_SUCCESS;
  }

  if (!vm.count("osmlr-tile-dir")) {
    std::cout << "You must provide a tile directory to read OSMLR tiles from.\n";
    return EXIT_FAILURE;
  }

  //queue up all the work we'll be doing
  std::deque<std::string> osmlr_tiles;
  auto itr = bfs::recursive_directory_iterator(tile_dir);
  auto end = bfs::recursive_directory_iterator();
  for (; itr != end; ++itr) {
    auto dir_entry = *itr;
    if (bfs::is_regular_file(dir_entry)) {
      auto ext = dir_entry.path().extension();
      if (ext == ".osmlr") {
        osmlr_tiles.emplace_back(dir_entry.path().string());
      }
    }
  }
  //std::random_shuffle(osmlr_tiles.begin(), osmlr_tiles.end());

  //configure logging
  vm::logging::Configure({{"type","std_err"},{"color","true"}});

  //parse the config
  bpt::ptree pt;
  bpt::read_json(config.c_str(), pt);

  //fire off some threads to do the work
  LOG_INFO("Associating local traffic segments with " + std::to_string(num_threads) + " threads");
  std::vector<std::shared_ptr<std::thread> > threads(num_threads);
  std::list<std::promise<edge_association> > results;
  std::mutex lock;
  for (auto& thread : threads) {
    results.emplace_back();
    thread.reset(new std::thread(add_local_associations, std::cref(pt), std::ref(osmlr_tiles),
                                 std::ref(lock), std::ref(results.back())));
  }

  //wait for it to finish
  for (auto& thread : threads)
    thread->join();
  LOG_INFO("Finished");

  //queue up the leftovers
  LOG_INFO("Associating neighbouring traffic segments with " + std::to_string(num_threads) + " threads");
  std::unordered_map<GraphId, leftovers_t> leftovers;
  for (auto& result : results) {
    auto associations = result.get_future().get();
    for(const auto& association : associations.leftovers()) {
      auto leftover = leftovers.insert({association.first.Tile_Base(), {}}).first;
      leftover->second.push_back(association);
    }
  }

  //write all the leftovers
  for (auto& thread : threads) {
    results.emplace_back();
    thread.reset(new std::thread(add_leftover_associations, std::cref(pt), std::ref(leftovers),
                                 std::ref(lock)));
  }

  //wait for it to finish
  for (auto& thread : threads)
    thread->join();
  LOG_INFO("Finished");

  return EXIT_SUCCESS;
}
