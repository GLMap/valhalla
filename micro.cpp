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

#include <valhalla/loki/worker.h>
#include <valhalla/thor/worker.h>
#include <valhalla/odin/worker.h>
#include <valhalla/tyr/serializers.h>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>

#pragma clang diagnostic pop

#include "micro.h"

__attribute__((visibility("default"))) std::variant<std::string, Error> computeRoute(std::string &&valhallaConfig, std::vector<std::string>&& tars, std::string &&json, bool optimize, const std::function<void()> &interrupt) {
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
        
        valhalla::loki::loki_worker_t loki_worker(config);
        valhalla::thor::thor_worker_t thor_worker(config);
        valhalla::odin::odin_worker_t odin_worker(config);
        
        loki_worker.set_interrupt(interrupt);
        thor_worker.set_interrupt(interrupt);
        odin_worker.set_interrupt(interrupt);
        
        valhalla::valhalla_request_t request;
        request.parse(json, optimize ? valhalla::odin::DirectionsOptions::optimized_route : valhalla::odin::DirectionsOptions::route);
        
        //check the request and locate the locations in the graph
        if (optimize)
            loki_worker.matrix(request);
        else
            loki_worker.route(request);
        //route between the locations in the graph to find the best path
        auto legs = optimize ? thor_worker.optimized_route(request) : thor_worker.route(request);
        //get some directions back from them
        auto directions = odin_worker.narrate(request, legs);
        //serialize them out to json string
        std::stringstream ss;
        ss << valhalla::tyr::serializeDirections(request, legs, directions);
        
        loki_worker.cleanup();
        thor_worker.cleanup();
        odin_worker.cleanup();
        
        return ss.str();
    }catch(const valhalla::valhalla_exception_t &ex)
    {
        return Error{ex.code, ex.message};
    }
}