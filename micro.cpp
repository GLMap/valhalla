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
#define EXPORT  __attribute__((visibility("default")))

#include <system_error>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <valhalla/loki/worker.h>
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
EXPORT void SetTZDataPath(const std::string &path) {
    date::set_install(path);
}
#endif

EXPORT void GetApproachAlert(const std::string& locale, bool imperial, double distance, const std::string& instruction, std::string &result) {
    valhalla::Options options;
    options.set_language(locale);
    options.set_units(imperial ? Options::miles : Options::kilometers);
    valhalla::odin::MarkupFormatter markup_formatter;
    auto builder = valhalla::odin::NarrativeBuilderFactory::Create(options, nullptr, markup_formatter);
    // принимает расстояние в милях или в километрах. в зависимости от юнитов, так что мы конвертим наши метры в то что он хочет
    result = builder->FormVerbalAlertApproachInstruction((distance / 1000.0) / (imperial ? midgard::kKmPerMile : 1), instruction);
}

EXPORT void Execute(const std::string &valhallaConfig, const std::vector<std::string> &tars, const std::function<int(const std::string &)> &fileOpenFunction,
                     const std::string &json, bool optimize, const std::function<void()> &interrupt, std::string &result) {
    try {
        boost::property_tree::ptree config;
        std::stringstream stream;
        stream << valhallaConfig;
        rapidjson::read_json(stream, config);
        boost::property_tree::ptree tile_extracts;
        for(const auto &path : tars) {
            boost::property_tree::ptree tile_extract;
            tile_extract.put("", path);
            tile_extracts.push_back(std::make_pair("", tile_extract));
        }
        config.add_child("mjolnir.tile_extracts", tile_extracts);

        auto graph_reader = std::make_shared<baldr::GraphReader>(config.get_child("mjolnir"), fileOpenFunction);

        valhalla::loki::loki_worker_t loki_worker(config, graph_reader);
        valhalla::thor::thor_worker_t thor_worker(config, graph_reader);
        valhalla::odin::odin_worker_t odin_worker(config);

        loki_worker.set_interrupt(&interrupt);
        thor_worker.set_interrupt(&interrupt);
        odin_worker.set_interrupt(&interrupt);

        valhalla::Api request;
        valhalla::ParseApi(json, optimize ? valhalla::Options::optimized_route : valhalla::Options::route, request);

        //check the request and locate the locations in the graph
        if (optimize)
            loki_worker.matrix(request);
        else
            loki_worker.route(request);
        //route between the locations in the graph to find the best path
        if (optimize) {
            thor_worker.optimized_route(request);
        } else {
            thor_worker.route(request);
        }
        //get some directions back from them
        odin_worker.narrate(request);
        //serialize them out to json string
        std::stringstream ss;
        ss << valhalla::tyr::serializeDirections(request);

        loki_worker.cleanup();
        thor_worker.cleanup();
        odin_worker.cleanup();

        result = ss.str();
    }catch(const valhalla_exception_t &ex)
    {
        throw std::system_error(ex.code, std::generic_category(), ex.message);
    }
}

void GetTimeRange(const std::string& condition, std::vector<uint64_t> &result) {
    result = mjolnir::get_time_range(condition);
}

}
}
