#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wcomma"
#pragma clang diagnostic ignored "-Wmacro-redefined"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wunused-private-field"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wnewline-eof"

#include "micro.h"
#define EXPORT __attribute__((visibility("default")))

#include <system_error>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <valhalla/loki/worker.h>
#include <valhalla/baldr/rapidjson_utils.h>
#include <valhalla/mjolnir/timeparsing.h>
#include <valhalla/odin/markup_formatter.h>
#include <valhalla/odin/narrative_builder_factory.h>
#include <valhalla/odin/worker.h>
#include <valhalla/thor/worker.h>
#include <valhalla/tyr/serializers.h>

#pragma clang diagnostic pop

extern "C" {
namespace valhalla {

#if !USE_OS_TZDB
EXPORT void SetTZDataPath(const std::string& path) {
  date::set_install(path);
}
#endif

EXPORT void GetApproachAlert(const std::string& locale,
                             bool imperial,
                             double distance,
                             const std::string& instruction,
                             std::string& result) {
  valhalla::Options options;
  options.set_language(locale);
  options.set_units(imperial ? Options::miles : Options::kilometers);
  valhalla::odin::MarkupFormatter markup_formatter;
  auto builder = valhalla::odin::NarrativeBuilderFactory::Create(options, nullptr, markup_formatter);
  // принимает расстояние в милях или в километрах. в зависимости от юнитов, так что мы конвертим наши
  // метры в то что он хочет
  result = builder->FormVerbalAlertApproachInstruction((distance / 1000.0) /
                                                           (imperial ? midgard::kKmPerMile : 1),
                                                       instruction);
}

bool Execute(const std::string& action,
             const std::string& json_query,
             const std::string& config,
             const std::vector<std::function<int(void)>>& tars,
             const std::function<void()>* interrupt,
             std::string& result) {
  boost::property_tree::ptree config_ptree;
  std::stringstream stream;
  stream << config;
  rapidjson::read_json(stream, config_ptree);

  // create our graph reader
  auto graph_reader = baldr::GraphReader(config_ptree.get_child("mjolnir"), tars);

  // setup an object that can answer the request
  valhalla::tyr::actor_t actor(config_ptree, graph_reader);

  // figure out which action
  valhalla::Options::Action action_enum;
  if (!valhalla::Options_Action_Enum_Parse(action, &action_enum)) {
    result = "Unknown action";
    return false;
  }

  // do the right action
  valhalla::Api request;
  try {
    switch (action_enum) {
      case valhalla::Options::route:
        result = actor.route(json_query, interrupt, &request);
        break;
      case valhalla::Options::locate:
        result = actor.locate(json_query, interrupt, &request);
        break;
      case valhalla::Options::sources_to_targets:
        result = actor.matrix(json_query, interrupt, &request);
        break;
      case valhalla::Options::optimized_route:
        result = actor.optimized_route(json_query, interrupt, &request);
        break;
      case valhalla::Options::isochrone:
        result = actor.isochrone(json_query, interrupt, &request);
        break;
      case valhalla::Options::trace_route:
        result = actor.trace_route(json_query, interrupt, &request);
        break;
      case valhalla::Options::trace_attributes:
        result = actor.trace_attributes(json_query, interrupt, &request);
        break;
      case valhalla::Options::height:
        result = actor.height(json_query, interrupt, &request);
        break;
      case valhalla::Options::transit_available:
        result = actor.transit_available(json_query, interrupt, &request);
        break;
      case valhalla::Options::expansion:
        result = actor.expansion(json_query, interrupt, &request);
        break;
      case valhalla::Options::status:
        result = actor.status(json_query, interrupt, &request);
        break;
      default:
        result = "Unknown action";
        return false;
    }
  } // request processing error specific error condition
  catch (const valhalla::valhalla_exception_t& ve) {
    result = valhalla::serialize_error(ve, request);
    return false;
  } // it was a regular exception!?
  catch (const std::exception& e) {
    result = serialize_error({599, std::string(e.what())}, request);
    return false;
  } // anything else
  catch (...) {
    result = serialize_error({599, std::string("Unknown exception thrown")}, request);
    return false;
  }
  return true;
}

void GetTimeRange(const std::string& condition, std::vector<uint64_t>& result) {
  result = mjolnir::get_time_range(condition);
}

} // namespace valhalla
}
