#include <cmath>
#include <cstdlib>
#include <iostream>

#include "midgard/constants.h"
#include "midgard/util.h"
#include "worker.h"

#include "odin/enhancedtrippath.h"
#include "odin/util.h"

#include <valhalla/proto/trippath.pb.h>

using namespace valhalla::midgard;

namespace {
const std::string& TripPath_RoadClass_Name(int v) {
  static const std::unordered_map<int, std::string> values{
      {0, "kMotorway"}, {1, "kTrunk"},        {2, "kPrimary"},     {3, "kSecondary"},
      {4, "kTertiary"}, {5, "kUnclassified"}, {6, "kResidential"}, {7, "kServiceOther"},
  };
  auto f = values.find(v);
  if (f == values.cend())
    throw std::runtime_error("Missing value in protobuf enum to string");
  return f->second;
}

const std::string& TripPath_Traversability_Name(int v) {
  static const std::unordered_map<int, std::string> values{
      {0, "kNone"},
      {1, "kForward"},
      {2, "kward"},
      {3, "kBoth"},
  };
  auto f = values.find(v);
  if (f == values.cend())
    throw std::runtime_error("Missing value in protobuf enum to string");
  return f->second;
}

const std::string& TripPath_Use_Name(int v) {
  static const std::unordered_map<int, std::string> values{
      {0, "kRoadUse"},
      {1, "kRampUse"},
      {2, "kTurnChannelUse"},
      {3, "kUse"},
      {4, "kDrivewayUse"},
      {5, "kAlleyUse"},
      {6, "kingAisleUse"},
      {7, "kEmergencyAccessUse"},
      {8, "kDriveThruUse"},
      {9, "kCuldesacUse"},
      {20, "kCyclewayUse"},
      {21, "keUse"},
      {24, "kUse"},
      {25, "kFootwayUse"},
      {26, "kStepsUse"},
      {27, "kPathUse"},
      {28, "kPedestrianUse"},
      {29, "kBridlewayUse"},
      {40, "kOtherUse"},
      {41, "kFerryUse"},
      {42, "kRailFerryUse"},
      {50, "kRailUse"},
      {51, "kBusUse"},
      {52, "kEgressConnectionUse"},
      {53, "kPlatformConnectionUse"},
      {54, "kTransitConnectionUse"},
  };
  auto f = values.find(v);
  if (f == values.cend())
    throw std::runtime_error("Missing value in protobuf enum to string");
  return f->second;
}

const std::string& TripPath_TravelMode_Name(int v) {
  static const std::unordered_map<int, std::string> values{
      {0, "kDrive"},
      {1, "kPedestrian"},
      {2, "kBicycle"},
      {3, "kTransit"},
  };
  auto f = values.find(v);
  if (f == values.cend())
    throw std::runtime_error("Missing value in protobuf enum to string");
  return f->second;
}

const std::string& TripPath_VehicleType_Name(int v) {
  static const std::unordered_map<int, std::string> values{
      {0, "kCar"}, {1, "kMotorcycle"}, {2, "kAutoBus"}, {3, "kTractorTrailer"}, {4, "kMotorScooter"},
  };
  auto f = values.find(v);
  if (f == values.cend())
    throw std::runtime_error("Missing value in protobuf enum to string");
  return f->second;
}

const std::string& TripPath_PedestrianType_Name(int v) {
  static const std::unordered_map<int, std::string> values{
      {0, "kFoot"},
      {1, "kWheelchair"},
      {2, "kSegway"},
  };
  auto f = values.find(v);
  if (f == values.cend())
    throw std::runtime_error("Missing value in protobuf enum to string");
  return f->second;
}

const std::string& TripPath_BicycleType_Name(int v) {
  static const std::unordered_map<int, std::string> values{
      {0, "kRoad"},
      {1, "kCross"},
      {2, "kHybrid"},
      {3, "kMountain"},
  };
  auto f = values.find(v);
  if (f == values.cend())
    throw std::runtime_error("Missing value in protobuf enum to string");
  return f->second;
}

const std::string& TripPath_TransitType_Name(int v) {
  static const std::unordered_map<int, std::string> values{
      {0, "kTram"},  {1, "kMetro"},    {2, "kRail"},    {3, "kBus"},
      {4, "kFerry"}, {5, "kCableCar"}, {6, "kGondola"}, {7, "kFunicular"},
  };
  auto f = values.find(v);
  if (f == values.cend())
    throw std::runtime_error("Missing value in protobuf enum to string");
  return f->second;
}

const std::string& TripPath_CycleLane_Name(int v) {
  static const std::unordered_map<int, std::string> values{
      {0, "kNoCycleLane"},
      {1, "kShared"},
      {2, "kDedicated"},
      {3, "kSeparated"},
  };
  auto f = values.find(v);
  if (f == values.cend())
    throw std::runtime_error("Missing value in protobuf enum to string");
  return f->second;
}

const std::string& TripPath_Sidewalk_Name(int v) {
  static const std::unordered_map<int, std::string> values{
      {0, "kNoSidewalk"},
      {1, "kLeft"},
      {2, "kRight"},
      {3, "kBothSides"},
  };
  auto f = values.find(v);
  if (f == values.cend())
    throw std::runtime_error("Missing value in protobuf enum to string");
  return f->second;
}

} // namespace

namespace valhalla {
namespace odin {

///////////////////////////////////////////////////////////////////////////////
// EnhancedTripPath

// EnhancedTripPath_Node* EnhancedTripPath::GetEnhancedNode(const int node_index) {
//   return static_cast<EnhancedTripPath_Node*>(mutable_node(node_index));
// }

static bool IsValidNodeIndex(const TripPath *tripPath, int node_index) {
  if ((node_index >= 0) && (node_index < tripPath->node_size())) {
    return true;
  }
  return false;
}

static bool IsLastNodeIndex(const TripPath *tripPath, int node_index) {
  if (IsValidNodeIndex(tripPath, node_index) && (node_index == (tripPath->node_size() - 1))) {
    return true;
  }
  return false;
}

TripPath_Edge* TripPath_GetPrevEdge(TripPath *tripPath, const int node_index, int delta) {
  int index = node_index - delta;
  if (IsValidNodeIndex(tripPath, index)) {
    return tripPath->mutable_node(index)->mutable_edge();
  } else {
    return nullptr;
  }
}

TripPath_Edge* TripPath_GetNextEdge(TripPath *tripPath, const int node_index, int delta) {
  int index = node_index + delta;
  if (IsValidNodeIndex(tripPath, index) && !IsLastNodeIndex(tripPath, index)) {
    return tripPath->mutable_node(index)->mutable_edge();
  } else {
    return nullptr;
  }
}

TripPath_Edge* TripPath_GetCurrEdge(TripPath *tripPath, const int node_index) {
  return TripPath_GetNextEdge(tripPath, node_index, 0);
}

int TripPath_GetLastNodeIndex(const TripPath *tripPath) {
  return (tripPath->node_size() - 1);
}

TripPath_Admin* TripPath_GetAdmin(TripPath *tripPath, size_t index) {
  return tripPath->mutable_admin(index);
}

std::string TripPath_GetCountryCode(TripPath *tripPath, int node_index) {
  return TripPath_GetAdmin(tripPath, tripPath->node(node_index).admin_index())->country_code();
}

std::string TripPath_GetStateCode(TripPath *tripPath, int node_index) {
  return TripPath_GetAdmin(tripPath, tripPath->node(node_index).admin_index())->state_code();
}

const ::valhalla::odin::Location& TripPath_GetOrigin(const TripPath *tripPath) {
  // Validate location count
  if (tripPath->location_size() < 2) {
    throw valhalla_exception_t{212};
  }

  return tripPath->location(0);
}

const ::valhalla::odin::Location& TripPath_GetDestination(const TripPath *tripPath) {
  // Validate location count
  if (tripPath->location_size() < 2) {
    throw valhalla_exception_t{212};
  }

  return tripPath->location(tripPath->location_size() - 1);
}

float TripPath_GetLength(const TripPath *tripPath, const DirectionsOptions::Units& units) {
  float length = 0.0f;
  for (const auto& n : tripPath->node()) {
    if (n.has_edge()) {
      length += n.edge().length();
    }
  }
  if (units == DirectionsOptions::miles) {
    return (length * kMilePerKm);
  }
  return length;
}

///////////////////////////////////////////////////////////////////////////////
// EnhancedTripPath_Edge

bool TripPath_Edge_IsUnnamed(const TripPath_Edge *edge) {
  return (edge->name_size() == 0);
}

bool TripPath_Edge_IsRoadUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kRoadUse);
}

bool TripPath_Edge_IsRampUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kRampUse);
}

bool TripPath_Edge_IsTurnChannelUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kTurnChannelUse);
}

bool TripPath_Edge_IsTrackUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kTrackUse);
}

bool TripPath_Edge_IsDrivewayUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kDrivewayUse);
}

bool TripPath_Edge_IsAlleyUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kAlleyUse);
}

bool TripPath_Edge_IsParkingAisleUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kParkingAisleUse);
}

bool TripPath_Edge_IsEmergencyAccessUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kEmergencyAccessUse);
}

bool TripPath_Edge_IsDriveThruUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kDriveThruUse);
}

bool TripPath_Edge_IsCuldesacUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kCuldesacUse);
}

bool TripPath_Edge_IsCyclewayUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kCyclewayUse);
}

bool TripPath_Edge_IsMountainBikeUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kMountainBikeUse);
}

bool TripPath_Edge_IsSidewalkUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kSidewalkUse);
}

bool TripPath_Edge_IsFootwayUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kFootwayUse);
}

bool TripPath_Edge_IsStepsUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kStepsUse);
}

bool TripPath_Edge_IsPathUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kPathUse);
}

bool TripPath_Edge_IsPedestrianUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kPedestrianUse);
}

bool TripPath_Edge_IsBridlewayUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kBridlewayUse);
}

bool TripPath_Edge_IsOtherUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kOtherUse);
}

bool TripPath_Edge_IsFerryUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kFerryUse);
}

bool TripPath_Edge_IsRailFerryUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kRailFerryUse);
}

bool TripPath_Edge_IsRailUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kRailUse);
}

bool TripPath_Edge_IsBusUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kBusUse);
}

bool TripPath_Edge_IsEgressConnectionUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kEgressConnectionUse);
}

bool TripPath_Edge_IsPlatformConnectionUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kPlatformConnectionUse);
}

bool TripPath_Edge_IsTransitConnectionUse(const TripPath_Edge *edge) {
  return (edge->use() == TripPath_Use_kTransitConnectionUse);
}

bool TripPath_Edge_IsTransitConnection(const TripPath_Edge *edge) {
  return (TripPath_Edge_IsTransitConnectionUse(edge) || TripPath_Edge_IsEgressConnectionUse(edge) || TripPath_Edge_IsPlatformConnectionUse(edge));
}

bool TripPath_Edge_IsUnnamedWalkway(const TripPath_Edge *edge) {
  return (TripPath_Edge_IsUnnamed(edge) && TripPath_Edge_IsFootwayUse(edge));
}

bool TripPath_Edge_IsUnnamedCycleway(const TripPath_Edge *edge) {
  return (TripPath_Edge_IsUnnamed(edge) && TripPath_Edge_IsCyclewayUse(edge));
}

bool TripPath_Edge_IsUnnamedMountainBikeTrail(const TripPath_Edge *edge) {
  return (TripPath_Edge_IsUnnamed(edge) && TripPath_Edge_IsMountainBikeUse(edge));
}

bool TripPath_Edge_IsHighway(const TripPath_Edge *edge) {
  return ((edge->road_class() == TripPath_RoadClass_kMotorway) && (!TripPath_Edge_IsRampUse(edge)));
}

bool TripPath_Edge_IsOneway(const TripPath_Edge *edge) {
  return ((edge->traversability() == TripPath_Traversability_kForward) ||
          (edge->traversability() == TripPath_Traversability_kBackward));
}

bool TripPath_Edge_IsForward(uint32_t prev2curr_turn_degree) {
  return ((prev2curr_turn_degree > 314) || (prev2curr_turn_degree < 46));
}

bool TripPath_Edge_IsWiderForward(uint32_t prev2curr_turn_degree) {
  return ((prev2curr_turn_degree > 304) || (prev2curr_turn_degree < 56));
}

bool TripPath_Edge_IsStraightest(const TripPath_Edge *edge, uint32_t prev2curr_turn_degree,
                                          uint32_t straightest_xedge_turn_degree) {
  if (TripPath_Edge_IsWiderForward(prev2curr_turn_degree)) {
    int path_xedge_turn_degree_delta = std::abs(static_cast<int>(prev2curr_turn_degree) -
                                                static_cast<int>(straightest_xedge_turn_degree));
    if (path_xedge_turn_degree_delta > 180) {
      path_xedge_turn_degree_delta = (360 - path_xedge_turn_degree_delta);
    }
    uint32_t path_straight_delta =
        (prev2curr_turn_degree > 180) ? (360 - prev2curr_turn_degree) : prev2curr_turn_degree;
    uint32_t xedge_straight_delta = (straightest_xedge_turn_degree > 180)
                                        ? (360 - straightest_xedge_turn_degree)
                                        : straightest_xedge_turn_degree;
    return ((path_xedge_turn_degree_delta > 10) ? (path_straight_delta <= xedge_straight_delta)
                                                : true);
  } else {
    return false;
  }
}

std::vector<std::string> TripPath_Edge_GetNameList(const TripPath_Edge *edge) {
  std::vector<std::string> name_list;
  for (const auto& name : edge->name()) {
    name_list.push_back(name);
  }
  return name_list;
}

float TripPath_Edge_GetLength(const TripPath_Edge *edge, const DirectionsOptions::Units& units) {
  if (units == DirectionsOptions::miles) {
    return (edge->length() * kMilePerKm);
  }
  return edge->length();
}

#ifdef LOGGING_LEVEL_TRACE
std::string EnhancedTripPath_Edge::ToString() const {
  std::string str;
  str.reserve(256);

  str += "name=";
  if (name_size() == 0) {
    str += "unnamed";
  } else {
    str += ListToString(this->name());
  }

  str += " | length=";
  str += std::to_string(length());

  str += " | speed=";
  str += std::to_string(speed());

  str += " | road_class=";
  str += std::to_string(road_class());

  str += " | begin_heading=";
  str += std::to_string(begin_heading());

  str += " | end_heading=";
  str += std::to_string(end_heading());

  str += " | begin_shape_index=";
  str += std::to_string(begin_shape_index());

  str += " | end_shape_index=";
  str += std::to_string(end_shape_index());

  str += " | traversability=";
  str += std::to_string(traversability());

  str += " | use=";
  str += std::to_string(use());

  str += " | toll=";
  str += std::to_string(toll());

  str += " | unpaved=";
  str += std::to_string(unpaved());

  str += " | tunnel=";
  str += std::to_string(tunnel());

  str += " | bridge=";
  str += std::to_string(bridge());

  str += " | roundabout=";
  str += std::to_string(roundabout());

  str += " | internal_intersection=";
  str += std::to_string(internal_intersection());

  // Process exits, if needed
  if (this->has_sign()) {
    str += " | exit.number=";
    str += ListToString(this->sign().exit_number());

    str += " | exit.branch=";
    str += ListToString(this->sign().exit_branch());

    str += " | exit.toward=";
    str += ListToString(this->sign().exit_toward());

    str += " | exit.name=";
    str += ListToString(this->sign().exit_name());
  }

  str += " | travel_mode=";
  str += std::to_string(travel_mode());

  // NOTE: Current PopulateEdge implementation

  str += " | vehicle_type=";
  str += std::to_string(vehicle_type());

  str += " | pedestrian_type=";
  str += std::to_string(pedestrian_type());

  str += " | bicycle_type=";
  str += std::to_string(bicycle_type());

  str += " | transit_type=";
  str += std::to_string(transit_type());

  str += " | drive_on_right=";
  str += std::to_string(drive_on_right());

  str += " | surface=";
  str += std::to_string(surface());

  // Process transit route info, if needed
  if (has_transit_route_info()) {
    str += " | transit_route_info.onestop_id=";
    str += transit_route_info().onestop_id();

    str += " | transit_route_info.block_id=";
    str += std::to_string(transit_route_info().block_id());

    str += " | transit_route_info.trip_id=";
    str += std::to_string(transit_route_info().trip_id());

    str += " | transit_route_info.short_name=";
    str += transit_route_info().short_name();

    str += " | transit_route_info.long_name=";
    str += transit_route_info().long_name();

    str += " | transit_route_info.headsign=";
    str += transit_route_info().headsign();

    str += " | transit_route_info.color=";
    str += std::to_string(transit_route_info().color());

    str += " | transit_route_info.text_color=";
    str += std::to_string(transit_route_info().text_color());

    str += " | transit_route_info.description=";
    str += transit_route_info().description();

    str += " | transit_route_info.operator_onestop_id=";
    str += transit_route_info().operator_onestop_id();

    str += " | transit_route_info.operator_name=";
    str += transit_route_info().operator_name();

    str += " | transit_route_info.operator_url=";
    str += transit_route_info().operator_url();
  }

  str += " | id=";
  str += std::to_string(id());

  str += " | way_id=";
  str += std::to_string(way_id());

  str += " | weighted_grade=";
  str += std::to_string(weighted_grade());

  str += " | max_upward_grade=";
  str += std::to_string(max_upward_grade());

  str += " | max_downward_grade=";
  str += std::to_string(max_downward_grade());

  str += " | lane_count=";
  str += std::to_string(lane_count());

  str += " | cycle_lane=";
  str += std::to_string(cycle_lane());

  str += " | bicycle_network=";
  str += std::to_string(bicycle_network());

  str += " | sidewalk=";
  str += std::to_string(sidewalk());

  str += " | density=";
  str += std::to_string(density());

  str += " | speed_limit=";
  str += std::to_string(speed_limit());

  str += " | truck_speed=";
  str += std::to_string(truck_speed());

  str += " | truck_route=";
  str += std::to_string(truck_route());

  return str;
}

std::string EnhancedTripPath_Edge::ToParameterString() const {
  const std::string delim = ", ";
  std::string str;
  str.reserve(128);

  str += ListToParameterString(this->name());

  str += delim;
  str += std::to_string(length());

  str += delim;
  str += std::to_string(speed());

  str += delim;
  str += "TripPath_RoadClass_";
  str += TripPath_RoadClass_Name(road_class());

  str += delim;
  str += std::to_string(begin_heading());

  str += delim;
  str += std::to_string(end_heading());

  str += delim;
  str += std::to_string(begin_shape_index());

  str += delim;
  str += std::to_string(end_shape_index());

  str += delim;
  str += "TripPath_Traversability_";
  str += TripPath_Traversability_Name(traversability());

  str += delim;
  str += "TripPath_Use_";
  str += TripPath_Use_Name(use());

  str += delim;
  str += std::to_string(toll());

  str += delim;
  str += std::to_string(unpaved());

  str += delim;
  str += std::to_string(tunnel());

  str += delim;
  str += std::to_string(bridge());

  str += delim;
  str += std::to_string(roundabout());

  str += delim;
  str += std::to_string(internal_intersection());

  str += delim;
  str += ListToParameterString(this->sign().exit_number());

  str += delim;
  str += ListToParameterString(this->sign().exit_branch());

  str += delim;
  str += ListToParameterString(this->sign().exit_toward());

  str += delim;
  str += ListToParameterString(this->sign().exit_name());

  str += delim;
  if (this->has_travel_mode()) {
    str += "TripPath_TravelMode_";
    str += TripPath_TravelMode_Name(travel_mode());
  }

  // NOTE: Current PopulateEdge implementation

  str += delim;
  if (this->has_vehicle_type()) {
    str += "TripPath_VehicleType_";
    str += TripPath_VehicleType_Name(vehicle_type());
  }

  str += delim;
  if (this->has_pedestrian_type()) {
    str += "TripPath_PedestrianType_";
    str += TripPath_PedestrianType_Name(pedestrian_type());
  }

  str += delim;
  if (this->has_bicycle_type()) {
    str += "TripPath_BicycleType_";
    str += TripPath_BicycleType_Name(bicycle_type());
  }

  str += delim;
  if (this->has_transit_type()) {
    str += "TripPath_TransitType_";
    str += TripPath_TransitType_Name(transit_type());
  }

  str += delim;
  str += std::to_string(drive_on_right());

  str += delim;
  str += std::to_string(surface());

  str += delim;
  if (transit_route_info().has_onestop_id()) {
    str += "\"";
    str += transit_route_info().onestop_id();
    str += "\"";
  }

  str += delim;
  str += std::to_string(transit_route_info().block_id());

  str += delim;
  str += std::to_string(transit_route_info().trip_id());

  str += delim;
  if (transit_route_info().has_short_name()) {
    str += "\"";
    str += transit_route_info().short_name();
    str += "\"";
  }

  str += delim;
  if (transit_route_info().has_long_name()) {
    str += "\"";
    str += transit_route_info().long_name();
    str += "\"";
  }

  str += delim;
  if (transit_route_info().has_headsign()) {
    str += "\"";
    str += transit_route_info().headsign();
    str += "\"";
  }

  str += delim;
  str += std::to_string(transit_route_info().color());

  str += delim;
  str += std::to_string(transit_route_info().text_color());

  str += delim;
  if (transit_route_info().has_operator_onestop_id()) {
    str += "\"";
    str += transit_route_info().operator_onestop_id();
    str += "\"";
  }

  str += delim;
  str += std::to_string(id());

  str += delim;
  str += std::to_string(way_id());

  str += delim;
  str += std::to_string(weighted_grade());

  str += delim;
  str += std::to_string(max_upward_grade());

  str += delim;
  str += std::to_string(max_downward_grade());

  str += delim;
  str += std::to_string(lane_count());

  str += delim;
  str += "TripPath_CycleLane_";
  str += TripPath_CycleLane_Name(cycle_lane());

  str += delim;
  str += std::to_string(bicycle_network());

  str += delim;
  str += "TripPath_Sidewalk_";
  str += TripPath_Sidewalk_Name(sidewalk());

  str += delim;
  str += std::to_string(density());

  str += delim;
  str += std::to_string(speed_limit());

  str += delim;
  str += std::to_string(truck_speed());

  str += delim;
  str += std::to_string(truck_route());

  return str;
}

std::string EnhancedTripPath_Edge::ListToString(
    const ::google::protobuf::RepeatedPtrField<::std::string>& string_list) const {
  std::string str;

  bool is_first = true;
  for (const auto& item : string_list) {
    if (is_first) {
      is_first = false;
    } else {
      str += "/";
    }
    str += item;
  }
  return str;
}

std::string EnhancedTripPath_Edge::ListToParameterString(
    const ::google::protobuf::RepeatedPtrField<::std::string>& string_list) const {
  std::string str;

  str += "{ ";
  bool is_first = true;
  for (const auto& item : string_list) {
    if (is_first) {
      is_first = false;
    } else {
      str += ", ";
    }
    str += "\"";
    str += item;
    str += "\"";
  }
  str += " }";

  return str;
}
#endif

///////////////////////////////////////////////////////////////////////////////
// EnhancedTripPath_IntersectingEdge

bool TripPath_IntersectingEdge_IsTraversable(
  const TripPath_IntersectingEdge* edge,
  const TripPath_TravelMode travel_mode) {
  TripPath_Traversability t;

  // Set traversability based on travel mode
  if (travel_mode == TripPath_TravelMode_kDrive) {
    t = edge->driveability();
  } else if (travel_mode == TripPath_TravelMode_kBicycle) {
    t = edge->cyclability();
  } else {
    t = edge->walkability();
  }

  if (t != TripPath_Traversability_kNone) {
    return true;
  }
  return false;
}

bool TripPath_IntersectingEdge_IsTraversableOutbound(
    const TripPath_IntersectingEdge* edge,
    const TripPath_TravelMode travel_mode) {
  TripPath_Traversability t;

  // Set traversability based on travel mode
  if (travel_mode == TripPath_TravelMode_kDrive) {
    t = edge->driveability();
  } else if (travel_mode == TripPath_TravelMode_kBicycle) {
    t = edge->cyclability();
  } else {
    t = edge->walkability();
  }

  if ((t == TripPath_Traversability_kForward) || (t == TripPath_Traversability_kBoth)) {
    return true;
  }
  return false;
}

std::string TripPath_IntersectingEdge_ToString(const TripPath_IntersectingEdge* edge) {
  std::string str;
  str.reserve(128);

  str += "begin_heading=";
  str += std::to_string(edge->begin_heading());

  str += " | prev_name_consistency=";
  str += std::to_string(edge->prev_name_consistency());

  str += " | curr_name_consistency=";
  str += std::to_string(edge->curr_name_consistency());

  str += " | driveability=";
  str += std::to_string(edge->driveability());

  str += " | cyclability=";
  str += std::to_string(edge->cyclability());

  str += " | walkability=";
  str += std::to_string(edge->walkability());

  return str;
}

///////////////////////////////////////////////////////////////////////////////
// EnhancedTripPath_Node

bool TripPath_Node_HasIntersectingEdges(const TripPath_Node *node) {
  return (node->intersecting_edge_size() > 0);
}

bool TripPath_Node_HasIntersectingEdgeNameConsistency(const TripPath_Node *node) {
  for (const auto& xedge : node->intersecting_edge()) {
    if (xedge.curr_name_consistency() || xedge.prev_name_consistency()) {
      return true;
    }
  }
  return false;
}

TripPath_IntersectingEdge* TripPath_Node_GetIntersectingEdge(TripPath_Node *node, size_t index) {
  return node->mutable_intersecting_edge(index);
}

void TripPath_Node_CalculateRightLeftIntersectingEdgeCounts(
    TripPath_Node *node,
    uint32_t from_heading,
    const TripPath_TravelMode travel_mode,
    IntersectingEdgeCounts& xedge_counts) {
  xedge_counts.clear();

  // No turn - just return
  if (node->intersecting_edge_size() == 0) {
    return;
  }

  uint32_t path_turn_degree = GetTurnDegree(from_heading, node->edge().begin_heading());
  for (int i = 0; i < node->intersecting_edge_size(); ++i) {
    uint32_t intersecting_turn_degree =
        GetTurnDegree(from_heading, node->intersecting_edge(i).begin_heading());
    bool xedge_traversable_outbound = TripPath_IntersectingEdge_IsTraversableOutbound(TripPath_Node_GetIntersectingEdge(node, i), travel_mode);

    if (path_turn_degree > 180) {
      if ((intersecting_turn_degree > path_turn_degree) || (intersecting_turn_degree < 180)) {
        ++xedge_counts.right;
        if (IsSimilarTurnDegree(path_turn_degree, intersecting_turn_degree, true)) {
          ++xedge_counts.right_similar;
          if (xedge_traversable_outbound) {
            ++xedge_counts.right_similar_traversable_outbound;
          }
        }
        if (xedge_traversable_outbound) {
          ++xedge_counts.right_traversable_outbound;
        }
      } else if ((intersecting_turn_degree < path_turn_degree) && (intersecting_turn_degree > 180)) {
        ++xedge_counts.left;
        if (IsSimilarTurnDegree(path_turn_degree, intersecting_turn_degree, false)) {
          ++xedge_counts.left_similar;
          if (xedge_traversable_outbound) {
            ++xedge_counts.left_similar_traversable_outbound;
          }
        }
        if (xedge_traversable_outbound) {
          ++xedge_counts.left_traversable_outbound;
        }
      }
    } else {
      if ((intersecting_turn_degree > path_turn_degree) && (intersecting_turn_degree < 180)) {
        ++xedge_counts.right;
        if (IsSimilarTurnDegree(path_turn_degree, intersecting_turn_degree, true)) {
          ++xedge_counts.right_similar;
          if (xedge_traversable_outbound) {
            ++xedge_counts.right_similar_traversable_outbound;
          }
        }
        if (xedge_traversable_outbound) {
          ++xedge_counts.right_traversable_outbound;
        }
      } else if ((intersecting_turn_degree < path_turn_degree) || (intersecting_turn_degree > 180)) {
        ++xedge_counts.left;
        if (IsSimilarTurnDegree(path_turn_degree, intersecting_turn_degree, false)) {
          ++xedge_counts.left_similar;
          if (xedge_traversable_outbound) {
            ++xedge_counts.left_similar_traversable_outbound;
          }
        }
        if (xedge_traversable_outbound) {
          ++xedge_counts.left_traversable_outbound;
        }
      }
    }
  }
}

bool TripPath_Node_HasFowardIntersectingEdge(TripPath_Node *node, uint32_t from_heading) {
  for (int i = 0; i < node->intersecting_edge_size(); ++i) {
    uint32_t intersecting_turn_degree =
        GetTurnDegree(from_heading, node->intersecting_edge(i).begin_heading());
    if ((intersecting_turn_degree > 314) || (intersecting_turn_degree < 46)) {
      return true;
    }
  }
  return false;
}

bool TripPath_Node_HasForwardTraversableIntersectingEdge(
    TripPath_Node *node,
    uint32_t from_heading,
    const TripPath_TravelMode travel_mode) {

  for (int i = 0; i < node->intersecting_edge_size(); ++i) {
    uint32_t intersecting_turn_degree =
        GetTurnDegree(from_heading, node->intersecting_edge(i).begin_heading());
    bool xedge_traversable_outbound = TripPath_IntersectingEdge_IsTraversableOutbound(TripPath_Node_GetIntersectingEdge(node, i), travel_mode);
    if (((intersecting_turn_degree > 314) || (intersecting_turn_degree < 46)) &&
        xedge_traversable_outbound) {
      return true;
    }
  }
  return false;
}

bool TripPath_Node_HasTraversableOutboundIntersectingEdge(
    TripPath_Node *node,
    const TripPath_TravelMode travel_mode) {

  for (int i = 0; i < node->intersecting_edge_size(); ++i) {
    if (TripPath_IntersectingEdge_IsTraversableOutbound(TripPath_Node_GetIntersectingEdge(node, i), travel_mode)) {
      return true;
    }
  }
  return false;
}

// TODO: refactor to clean up code
uint32_t TripPath_Node_GetStraightestTraversableIntersectingEdgeTurnDegree(
    TripPath_Node *node,
    uint32_t from_heading,
    const TripPath_TravelMode travel_mode) {

  uint32_t staightest_turn_degree = 180; // Initialize to reverse turn degree
  uint32_t staightest_delta = 180;       // Initialize to reverse delta

  for (int i = 0; i < node->intersecting_edge_size(); ++i) {
    uint32_t intersecting_turn_degree =
        GetTurnDegree(from_heading, node->intersecting_edge(i).begin_heading());
    bool xedge_traversable_outbound = TripPath_IntersectingEdge_IsTraversableOutbound(TripPath_Node_GetIntersectingEdge(node, i), travel_mode);
    uint32_t straight_delta = (intersecting_turn_degree > 180) ? (360 - intersecting_turn_degree)
                                                               : intersecting_turn_degree;
    if (xedge_traversable_outbound && (straight_delta < staightest_delta)) {
      staightest_delta = straight_delta;
      staightest_turn_degree = intersecting_turn_degree;
    }
  }
  return staightest_turn_degree;
}

uint32_t TripPath_Node_GetStraightestIntersectingEdgeTurnDegree(TripPath_Node *node, uint32_t from_heading) {

  uint32_t staightest_turn_degree = 180; // Initialize to reverse turn degree
  uint32_t staightest_delta = 180;       // Initialize to reverse delta

  for (int i = 0; i < node->intersecting_edge_size(); ++i) {
    uint32_t intersecting_turn_degree =
        GetTurnDegree(from_heading, node->intersecting_edge(i).begin_heading());
    uint32_t straight_delta = (intersecting_turn_degree > 180) ? (360 - intersecting_turn_degree)
                                                               : intersecting_turn_degree;
    if (straight_delta < staightest_delta) {
      staightest_delta = straight_delta;
      staightest_turn_degree = intersecting_turn_degree;
    }
  }
  return staightest_turn_degree;
}

bool TripPath_Node_IsStreetIntersection(const TripPath_Node *node) {
  return (node->type() == TripPath_Node_Type_kStreetIntersection);
}

bool TripPath_Node_IsGate(const TripPath_Node *node) {
  return (node->type() == TripPath_Node_Type_kGate);
}

bool TripPath_Node_IsBollard(const TripPath_Node *node) {
  return (node->type() == TripPath_Node_Type_kBollard);
}

bool TripPath_Node_IsTollBooth(const TripPath_Node *node) {
  return (node->type() == TripPath_Node_Type_kTollBooth);
}

bool TripPath_Node_IsTransitEgress(const TripPath_Node *node) {
  return (node->type() == TripPath_Node_Type_kTransitEgress);
}

bool TripPath_Node_IsTransitStation(const TripPath_Node *node) {
  return (node->type() == TripPath_Node_Type_kTransitStation);
}

bool TripPath_Node_IsTransitPlatform(const TripPath_Node *node) {
  return (node->type() == TripPath_Node_Type_kTransitPlatform);
}

bool TripPath_Node_IsBikeShare(const TripPath_Node *node) {
  return (node->type() == TripPath_Node_Type_kBikeShare);
}

bool TripPath_Node_IsParking(const TripPath_Node *node) {
  return (node->type() == TripPath_Node_Type_kParking);
}

bool TripPath_Node_IsMotorwayJunction(const TripPath_Node *node) {
  return (node->type() == TripPath_Node_Type_kMotorwayJunction);
}

bool TripPath_Node_IsBorderControl(const TripPath_Node *node) {
  return (node->type() == TripPath_Node_Type_kBorderControl);
}

std::string TripPath_Node_ToString(const TripPath_Node *node) {
  std::string str;
  str.reserve(256);

  str += "elapsed_time=";
  str += std::to_string(node->elapsed_time());

  str += " | admin_index=";
  str += std::to_string(node->admin_index());

  str += " | type=";
  str += std::to_string(node->type());

  str += " | fork=";
  str += std::to_string(node->fork());

  if (node->has_transit_platform_info()) {
    str += " | transit_platform_info.type=";
    str += std::to_string(node->transit_platform_info().type());

    str += " | transit_platform_info.onestop_id=";
    str += node->transit_platform_info().onestop_id();

    str += " | transit_platform_info.name=";
    str += node->transit_platform_info().name();

    str += " | transit_platform_info.arrival_date_time=";
    str += node->transit_platform_info().arrival_date_time();

    str += " | transit_platform_info.departure_date_time=";
    str += node->transit_platform_info().departure_date_time();

    str += " | transit_platform_info.assumed_schedule()=";
    str += std::to_string(node->transit_platform_info().assumed_schedule());

    str += " | transit_platform_info.station_onestop_id=";
    str += node->transit_platform_info().station_onestop_id();

    str += " | transit_platform_info.station_name=";
    str += node->transit_platform_info().station_name();
  }

  str += " | time_zone=";
  str += node->time_zone();

  return str;
}

///////////////////////////////////////////////////////////////////////////////
// EnhancedTripPath_Admin

// std::string EnhancedTripPath_Admin::ToString() const {
//   std::string str;
//   str.reserve(256);

//   str += "country_code=";
//   str += country_code();

//   str += " | country_text=";
//   str += country_text();

//   str += " | state_code=";
//   str += state_code();

//   str += " | state_text=";
//   str += state_text();

//   return str;
// }

} // namespace odin
} // namespace valhalla
