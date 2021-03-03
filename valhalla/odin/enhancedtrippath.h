#ifndef VALHALLA_ODIN_ENHANCEDTRIPPATH_H_
#define VALHALLA_ODIN_ENHANCEDTRIPPATH_H_

#include <cstdint>
#include <string>
#include <unordered_map>

#include <valhalla/proto/directions_options.pb.h>
#include <valhalla/proto/trippath.pb.h>

namespace valhalla {
namespace odin {

TripPath_Edge* TripPath_GetPrevEdge(TripPath *tripPath, const int node_index, int delta = 1);
TripPath_Edge* TripPath_GetCurrEdge(TripPath *tripPath, const int node_index);
TripPath_Edge* TripPath_GetNextEdge(TripPath *tripPath, const int node_index, int delta = 1);
float TripPath_GetLength(const TripPath *tripPath, const DirectionsOptions::Units& units);

int TripPath_GetLastNodeIndex(const TripPath *tripPath);
TripPath_Admin* TripPath_GetAdmin(TripPath *tripPath, size_t index);
std::string TripPath_GetCountryCode(TripPath *tripPath, int node_index);
std::string TripPath_GetStateCode(TripPath *tripPath, int node_index);

const ::valhalla::odin::Location& TripPath_GetOrigin(const TripPath *tripPath);
const ::valhalla::odin::Location& TripPath_GetDestination(const TripPath *tripPath);

bool TripPath_Edge_IsUnnamed(const TripPath_Edge *edge);
bool TripPath_Edge_IsRoadUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsRampUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsTurnChannelUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsTrackUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsDrivewayUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsAlleyUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsParkingAisleUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsEmergencyAccessUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsDriveThruUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsCuldesacUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsCyclewayUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsMountainBikeUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsSidewalkUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsFootwayUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsStepsUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsPathUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsPedestrianUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsBridlewayUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsOtherUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsFerryUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsRailFerryUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsRailUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsBusUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsEgressConnectionUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsPlatformConnectionUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsTransitConnectionUse(const TripPath_Edge *edge);
bool TripPath_Edge_IsTransitConnection(const TripPath_Edge *edge);
bool TripPath_Edge_IsUnnamedWalkway(const TripPath_Edge *edge);
bool TripPath_Edge_IsUnnamedCycleway(const TripPath_Edge *edge);
bool TripPath_Edge_IsUnnamedMountainBikeTrail(const TripPath_Edge *edge);
bool TripPath_Edge_IsHighway(const TripPath_Edge *edge);
bool TripPath_Edge_IsOneway(const TripPath_Edge *edge);
bool TripPath_Edge_IsForward(uint32_t prev2curr_turn_degree);
bool TripPath_Edge_IsWiderForward(uint32_t prev2curr_turn_degree);
bool TripPath_Edge_IsStraightest(const TripPath_Edge *edge, uint32_t prev2curr_turn_degree,
                                          uint32_t straightest_xedge_turn_degree);
std::vector<std::string> TripPath_Edge_GetNameList(const TripPath_Edge *edge);
float TripPath_Edge_GetLength(const TripPath_Edge *edge, const DirectionsOptions::Units& units);

bool TripPath_IntersectingEdge_IsTraversable(const TripPath_IntersectingEdge* edge, const TripPath_TravelMode travel_mode);
bool TripPath_IntersectingEdge_IsTraversableOutbound(const TripPath_IntersectingEdge* edge, const TripPath_TravelMode travel_mode);
std::string TripPath_IntersectingEdge_ToString(const TripPath_IntersectingEdge* edge);

struct IntersectingEdgeCounts {

  IntersectingEdgeCounts() {
    clear();
  }

  IntersectingEdgeCounts(uint32_t r,
                         uint32_t rs,
                         uint32_t rdo,
                         uint32_t rsdo,
                         uint32_t l,
                         uint32_t ls,
                         uint32_t ldo,
                         uint32_t lsdo)
      : right(r), right_similar(rs), right_traversable_outbound(rdo),
        right_similar_traversable_outbound(rsdo), left(l), left_similar(ls),
        left_traversable_outbound(ldo), left_similar_traversable_outbound(lsdo) {
  }

  void clear() {
    right = 0;
    right_similar = 0;
    right_traversable_outbound = 0;
    right_similar_traversable_outbound = 0;
    left = 0;
    left_similar = 0;
    left_traversable_outbound = 0;
    left_similar_traversable_outbound = 0;
  }

  uint32_t right;
  uint32_t right_similar;
  uint32_t right_traversable_outbound;
  uint32_t right_similar_traversable_outbound;
  uint32_t left;
  uint32_t left_similar;
  uint32_t left_traversable_outbound;
  uint32_t left_similar_traversable_outbound;
};


bool TripPath_Node_HasIntersectingEdges(const TripPath_Node *node);
bool TripPath_Node_HasIntersectingEdgeNameConsistency(const TripPath_Node *node);
TripPath_IntersectingEdge* TripPath_Node_GetIntersectingEdge(TripPath_Node *node, size_t index);
void TripPath_Node_CalculateRightLeftIntersectingEdgeCounts(
    TripPath_Node *node,
    uint32_t from_heading,
    const TripPath_TravelMode travel_mode,
    IntersectingEdgeCounts& xedge_counts);
bool TripPath_Node_HasFowardIntersectingEdge(TripPath_Node *node, uint32_t from_heading);
bool TripPath_Node_HasForwardTraversableIntersectingEdge(
    TripPath_Node *node,
    uint32_t from_heading,
    const TripPath_TravelMode travel_mode);
bool TripPath_Node_HasTraversableOutboundIntersectingEdge(
    TripPath_Node *node,
    const TripPath_TravelMode travel_mode);
uint32_t TripPath_Node_GetStraightestTraversableIntersectingEdgeTurnDegree(
    TripPath_Node *node,
    uint32_t from_heading,
    const TripPath_TravelMode travel_mode);
uint32_t TripPath_Node_GetStraightestIntersectingEdgeTurnDegree(TripPath_Node *node, uint32_t from_heading);

bool TripPath_Node_IsStreetIntersection(const TripPath_Node *node);
bool TripPath_Node_IsGate(const TripPath_Node *node);
bool TripPath_Node_IsBollard(const TripPath_Node *node);
bool TripPath_Node_IsTollBooth(const TripPath_Node *node);
bool TripPath_Node_IsTransitEgress(const TripPath_Node *node);
bool TripPath_Node_IsTransitStation(const TripPath_Node *node);
bool TripPath_Node_IsTransitPlatform(const TripPath_Node *node);
bool TripPath_Node_IsBikeShare(const TripPath_Node *node);
bool TripPath_Node_IsParking(const TripPath_Node *node);
bool TripPath_Node_IsMotorwayJunction(const TripPath_Node *node);
bool TripPath_Node_IsBorderControl(const TripPath_Node *node);
std::string TripPath_Node_ToString(const TripPath_Node *node); 

const std::unordered_map<uint8_t, std::string> TripPath_TravelMode_Strings{
    {static_cast<uint8_t>(TripPath_TravelMode_kDrive), "drive"},
    {static_cast<uint8_t>(TripPath_TravelMode_kPedestrian), "pedestrian"},
    {static_cast<uint8_t>(TripPath_TravelMode_kBicycle), "bicycle"},
    {static_cast<uint8_t>(TripPath_TravelMode_kTransit), "transit"},
};
inline std::string to_string(TripPath_TravelMode travel_mode) {
  auto i = TripPath_TravelMode_Strings.find(static_cast<uint8_t>(travel_mode));
  if (i == TripPath_TravelMode_Strings.cend()) {
    return "null";
  }
  return i->second;
}

const std::unordered_map<uint8_t, std::string> TripPath_VehicleType_Strings{
    {static_cast<uint8_t>(TripPath_VehicleType_kCar), "car"},
    {static_cast<uint8_t>(TripPath_VehicleType_kMotorcycle), "motorcycle"},
    {static_cast<uint8_t>(TripPath_VehicleType_kAutoBus), "bus"},
    {static_cast<uint8_t>(TripPath_VehicleType_kTractorTrailer), "tractor_trailer"},
};
inline std::string to_string(TripPath_VehicleType vehicle_type) {
  auto i = TripPath_VehicleType_Strings.find(static_cast<uint8_t>(vehicle_type));
  if (i == TripPath_VehicleType_Strings.cend()) {
    return "null";
  }
  return i->second;
}

const std::unordered_map<uint8_t, std::string> TripPath_PedestrianType_Strings{
    {static_cast<uint8_t>(TripPath_PedestrianType_kFoot), "foot"},
    {static_cast<uint8_t>(TripPath_PedestrianType_kWheelchair), "wheelchair"},
    {static_cast<uint8_t>(TripPath_PedestrianType_kSegway), "segway"},
};
inline std::string to_string(TripPath_PedestrianType pedestrian_type) {
  auto i = TripPath_PedestrianType_Strings.find(static_cast<uint8_t>(pedestrian_type));
  if (i == TripPath_PedestrianType_Strings.cend()) {
    return "null";
  }
  return i->second;
}

const std::unordered_map<uint8_t, std::string> TripPath_BicycleType_Strings{
    {static_cast<uint8_t>(TripPath_BicycleType_kRoad), "road"},
    {static_cast<uint8_t>(TripPath_BicycleType_kCross), "cross"},
    {static_cast<uint8_t>(TripPath_BicycleType_kHybrid), "hybrid"},
    {static_cast<uint8_t>(TripPath_BicycleType_kMountain), "mountain"},
};
inline std::string to_string(TripPath_BicycleType bicycle_type) {
  auto i = TripPath_BicycleType_Strings.find(static_cast<uint8_t>(bicycle_type));
  if (i == TripPath_BicycleType_Strings.cend()) {
    return "null";
  }
  return i->second;
}

const std::unordered_map<uint8_t, std::string> TripPath_Sidewalk_Strings = {
    {static_cast<uint8_t>(TripPath_Sidewalk_kNoSidewalk), "none"},
    {static_cast<uint8_t>(TripPath_Sidewalk_kLeft), "left"},
    {static_cast<uint8_t>(TripPath_Sidewalk_kRight), "right"},
    {static_cast<uint8_t>(TripPath_Sidewalk_kBothSides), "both"},
};
inline std::string to_string(TripPath_Sidewalk s) {
  auto i = TripPath_Sidewalk_Strings.find(static_cast<uint8_t>(s));
  if (i == TripPath_Sidewalk_Strings.cend()) {
    return "null";
  }
  return i->second;
}

const std::unordered_map<uint8_t, std::string> TripPath_Traversability_Strings = {
    {static_cast<uint8_t>(TripPath_Traversability_kNone), "none"},
    {static_cast<uint8_t>(TripPath_Traversability_kForward), "forward"},
    {static_cast<uint8_t>(TripPath_Traversability_kBackward), "backward"},
    {static_cast<uint8_t>(TripPath_Traversability_kBoth), "both"},
};
inline std::string to_string(TripPath_Traversability t) {
  auto i = TripPath_Traversability_Strings.find(static_cast<uint8_t>(t));
  if (i == TripPath_Traversability_Strings.cend()) {
    return "null";
  }
  return i->second;
}

} // namespace odin
} // namespace valhalla

#endif // VALHALLA_ODIN_ENHANCEDTRIPPATH_H_
