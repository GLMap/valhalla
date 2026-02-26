// valhalla_native — production stdin/stdout trace_attributes processor
// Links against libvalhalla_micro.a. One JSON request per line in, one JSON response per line out.
//
// Usage: ./valhalla_native <config.json> <tzdata_path>
//
// Protocol:
//   stdin  → one JSON line per request (same format as HTTP ?json= param)
//   stdout ← one JSON line per response (success or serialized error)
//   stderr ← Valhalla logs + structured status lines

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wcomma"
#pragma clang diagnostic ignored "-Wmacro-redefined"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wunused-private-field"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wnewline-eof"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <date/tz.h>
#include <valhalla/baldr/graphreader.h>
#include <valhalla/midgard/logging.h>
#include <valhalla/tyr/actor.h>
#include <valhalla/worker.h>

#pragma clang diagnostic pop

#include <atomic>
#include <csignal>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static constexpr size_t MAX_LINE_BYTES = 1 * 1024 * 1024; // 1 MB

static std::atomic<bool> shutdown_requested{false};

static void signal_handler(int) {
  shutdown_requested.store(true, std::memory_order_relaxed);
}

static std::string read_file(const std::string& path) {
  std::ifstream f(path);
  if (!f.is_open()) {
    std::cerr << "ERROR: cannot open " << path << "\n";
    std::exit(1);
  }
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <config.json> <tzdata_path>\n";
    return 1;
  }

  const std::string config_path = argv[1];
  const std::string tzdata_path = argv[2];

  // Signal handling — finish current request, then exit
  std::signal(SIGTERM, signal_handler);
  std::signal(SIGINT, signal_handler);

  // Force all Valhalla logs to stderr (singleton — first call wins)
  valhalla::midgard::logging::Configure({{"type", "std_err"}, {"color", "false"}});

  // Parse config
  boost::property_tree::ptree config;
  {
    std::string config_json = read_file(config_path);
    std::istringstream ss(config_json);
    boost::property_tree::read_json(ss, config);
  }
  date::set_install(tzdata_path);

  // Create GraphReader (mmaps tar once) + actor_t (creates workers once)
  auto graph_reader = valhalla::baldr::GraphReader(config.get_child("mjolnir"));
  valhalla::tyr::actor_t actor(config, graph_reader, /*auto_cleanup=*/true);

  // Emit ready status
  std::cerr << "{\"status\":\"ready\"}\n" << std::flush;

  // Main loop — one JSON line per request
  uint64_t requests = 0;
  uint64_t errors = 0;
  std::string line;

  while (!shutdown_requested.load(std::memory_order_relaxed) && std::getline(std::cin, line)) {
    // Skip empty/whitespace lines
    if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos) {
      continue;
    }

    // Reject oversized lines
    if (line.size() > MAX_LINE_BYTES) {
      std::cout << "{\"error\":\"line exceeds 1MB limit\",\"error_code\":599}" << '\n' << std::flush;
      ++requests;
      ++errors;
      continue;
    }

    ++requests;
    std::string result;
    valhalla::Api api;

    try {
      result = actor.trace_attributes(line, nullptr, &api);
    } catch (const valhalla::valhalla_exception_t& ve) {
      result = valhalla::serialize_error(ve, api);
      ++errors;
    } catch (const std::exception& e) {
      result = valhalla::serialize_error({599, std::string(e.what())}, api);
      ++errors;
    } catch (...) {
      result = valhalla::serialize_error({599, std::string("Unknown exception")}, api);
      ++errors;
    }

    std::cout << result << '\n' << std::flush;
  }

  // Emit done status
  std::cerr << "{\"status\":\"done\",\"requests\":" << requests
            << ",\"errors\":" << errors << "}\n" << std::flush;

  return 0;
}
