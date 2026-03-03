// valhalla_tracer — HTTP trace_attributes server.
// Links against libvalhalla_micro.a.
//
// Usage:
//   ./valhalla_tracer <config.json> <tzdata_path> <port> [<workers>]
//
// POST any path → trace_attributes, returns JSON response.
// One actor per worker thread, each with its own GraphReader + tile cache.
// The underlying tile_extract mmap is shared via GraphReader::get_extract_instance().
// Emits {"status":"ready","port":<port>} on stderr when listening.

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
#include <condition_variable>
#include <csignal>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

static constexpr size_t MAX_BODY_BYTES = 2 * 1024 * 1024; // 2 MB

static std::atomic<bool> shutdown_requested{false};
static int server_fd = -1; // for shutdown

static void signal_handler(int) {
  shutdown_requested.store(true, std::memory_order_relaxed);
  // Unblock accept() by closing the server socket.
  if (server_fd >= 0) {
    ::close(server_fd);
    server_fd = -1;
  }
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

// Process a trace_attributes request string, return (status_code, response_body).
static std::pair<int, std::string> process_request(valhalla::tyr::actor_t& actor,
                                                    const std::string& body) {
  valhalla::Api api;
  try {
    std::string result = actor.trace_attributes(body, nullptr, &api);
    return {200, result};
  } catch (const valhalla::valhalla_exception_t& ve) {
    return {ve.http_code, valhalla::serialize_error(ve, api)};
  } catch (const std::exception& e) {
    return {500, valhalla::serialize_error({599, std::string(e.what())}, api)};
  } catch (...) {
    return {500, valhalla::serialize_error({599, std::string("Unknown exception")}, api)};
  }
}

// ---------------------------------------------------------------------------
// HTTP server
// ---------------------------------------------------------------------------

// Thread-safe work queue for client file descriptors.
struct WorkQueue {
  std::mutex mtx;
  std::condition_variable cv;
  std::queue<int> fds;
  bool stopped = false;

  void push(int fd) {
    std::lock_guard<std::mutex> lock(mtx);
    fds.push(fd);
    cv.notify_one();
  }

  int pop() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&] { return stopped || !fds.empty(); });
    if (stopped && fds.empty()) return -1;
    int fd = fds.front();
    fds.pop();
    return fd;
  }

  void shutdown() {
    std::lock_guard<std::mutex> lock(mtx);
    stopped = true;
    cv.notify_all();
  }
};

static bool read_exact(int fd, void* buf, size_t n) {
  char* p = static_cast<char*>(buf);
  while (n > 0) {
    ssize_t r = ::read(fd, p, n);
    if (r <= 0) {
      if (r < 0 && errno == EINTR) continue;
      return false;
    }
    p += r;
    n -= r;
  }
  return true;
}

static bool write_all(int fd, const void* buf, size_t n) {
  const char* p = static_cast<const char*>(buf);
  while (n > 0) {
    ssize_t w = ::write(fd, p, n);
    if (w <= 0) {
      if (w < 0 && errno == EINTR) continue;
      return false;
    }
    p += w;
    n -= w;
  }
  return true;
}

// Read an HTTP request body from a client socket. Returns false on read error.
static bool read_http_body(int fd, std::string& body) {
  // Read headers into a buffer. Headers are small (< 2 KB typically).
  std::string raw;
  raw.reserve(4096);
  char chunk[2048];
  size_t header_end_pos = std::string::npos;

  while (header_end_pos == std::string::npos) {
    ssize_t n = ::read(fd, chunk, sizeof(chunk));
    if (n <= 0) return false;
    raw.append(chunk, n);
    header_end_pos = raw.find("\r\n\r\n");
    if (raw.size() > 16384) return false; // headers too large
  }

  // Extract Content-Length from headers.
  int content_length = 0;
  std::string headers_str = raw.substr(0, header_end_pos);
  // Case-insensitive search for Content-Length.
  std::string lower = headers_str;
  for (auto& c : lower) c = static_cast<char>(tolower(c));
  size_t cl_pos = lower.find("content-length:");
  if (cl_pos != std::string::npos) {
    size_t val_start = cl_pos + 15;
    while (val_start < lower.size() && lower[val_start] == ' ') val_start++;
    size_t val_end = lower.find("\r\n", val_start);
    if (val_end == std::string::npos) val_end = lower.size();
    content_length = std::stoi(headers_str.substr(val_start, val_end - val_start));
  }

  if (content_length <= 0) {
    body.clear();
    return true;
  }
  if (static_cast<size_t>(content_length) > MAX_BODY_BYTES) return false;

  // Copy any body bytes already read past the header.
  size_t body_offset = header_end_pos + 4;
  size_t body_have = (raw.size() > body_offset) ? raw.size() - body_offset : 0;

  body.resize(content_length);
  if (body_have > 0) {
    size_t copy = std::min(body_have, static_cast<size_t>(content_length));
    memcpy(&body[0], raw.data() + body_offset, copy);
  }

  // Read remaining body bytes.
  if (body_have < static_cast<size_t>(content_length)) {
    if (!read_exact(fd, &body[body_have], content_length - body_have)) return false;
  }

  return true;
}

static void write_http_response(int fd, int status, const std::string& body) {
  const char* status_text = "OK";
  switch (status) {
  case 200: status_text = "OK"; break;
  case 400: status_text = "Bad Request"; break;
  case 404: status_text = "Not Found"; break;
  case 500: status_text = "Internal Server Error"; break;
  default: status_text = "Error"; break;
  }

  std::string header = "HTTP/1.1 " + std::to_string(status) + " " + status_text +
                        "\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: " +
                        std::to_string(body.size()) +
                        "\r\n"
                        "Connection: close\r\n"
                        "\r\n";

  write_all(fd, header.data(), header.size());
  if (!body.empty()) {
    write_all(fd, body.data(), body.size());
  }
}

static void run_http_server(const boost::property_tree::ptree& config,
                            int port,
                            int num_workers) {
  // Create one actor per worker thread, each with its own GraphReader + tile cache.
  // The underlying tile_extract mmap is shared via GraphReader::get_extract_instance().
  std::vector<std::unique_ptr<valhalla::tyr::actor_t>> actors;
  for (int i = 0; i < num_workers; i++) {
    actors.push_back(
        std::make_unique<valhalla::tyr::actor_t>(config, /*auto_cleanup=*/true));
  }

  WorkQueue queue;
  std::atomic<uint64_t> total_requests{0};
  std::atomic<uint64_t> total_errors{0};

  // Start worker threads.
  std::vector<std::thread> threads;
  for (int i = 0; i < num_workers; i++) {
    threads.emplace_back([&queue, &actors, &total_requests, &total_errors, i]() {
      auto& actor = *actors[i];
      while (true) {
        int fd = queue.pop();
        if (fd < 0) break;

        std::string body;
        if (!read_http_body(fd, body)) {
          ::close(fd);
          continue;
        }

        total_requests.fetch_add(1, std::memory_order_relaxed);

        if (body.empty()) {
          write_http_response(fd, 400, "{\"error\":\"empty body\",\"error_code\":400}");
        } else {
          auto [status, result] = process_request(actor, body);
          if (status != 200) total_errors.fetch_add(1, std::memory_order_relaxed);
          write_http_response(fd, status, result);
        }

        ::close(fd);
      }
    });
  }

  // Create server socket.
  server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    std::cerr << "ERROR: socket(): " << strerror(errno) << "\n";
    std::exit(1);
  }

  int optval = 1;
  ::setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

  struct sockaddr_in addr {};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = htons(static_cast<uint16_t>(port));

  if (::bind(server_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
    std::cerr << "ERROR: bind(" << port << "): " << strerror(errno) << "\n";
    std::exit(1);
  }

  if (::listen(server_fd, 64) < 0) {
    std::cerr << "ERROR: listen(): " << strerror(errno) << "\n";
    std::exit(1);
  }

  // Ready signal.
  std::cerr << "{\"status\":\"ready\",\"port\":" << port << ",\"workers\":" << num_workers << "}\n"
            << std::flush;

  // Accept loop.
  while (!shutdown_requested.load(std::memory_order_relaxed)) {
    int client_fd = ::accept(server_fd, nullptr, nullptr);
    if (client_fd < 0) {
      if (shutdown_requested.load(std::memory_order_relaxed)) break;
      if (errno == EINTR) continue;
      std::cerr << "WARN: accept(): " << strerror(errno) << "\n";
      continue;
    }
    queue.push(client_fd);
  }

  // Shutdown.
  queue.shutdown();
  for (auto& t : threads) t.join();

  std::cerr << "{\"status\":\"done\",\"requests\":" << total_requests.load()
            << ",\"errors\":" << total_errors.load() << "}\n"
            << std::flush;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0] << " <config.json> <tzdata_path> <port> [<workers>]\n";
    return 1;
  }

  const std::string config_path = argv[1];
  const std::string tzdata_path = argv[2];
  int port = std::stoi(argv[3]);
  int num_workers = 8;
  if (argc >= 5) num_workers = std::stoi(argv[4]);

  if (port <= 0 || port > 65535) {
    std::cerr << "ERROR: invalid port " << port << "\n";
    return 1;
  }
  if (num_workers <= 0 || num_workers > 128) {
    std::cerr << "ERROR: workers must be 1..128, got " << num_workers << "\n";
    return 1;
  }

  std::signal(SIGTERM, signal_handler);
  std::signal(SIGINT, signal_handler);

  valhalla::midgard::logging::Configure({{"type", "std_err"}, {"color", "false"}});

  // Parse config.
  boost::property_tree::ptree config;
  {
    std::string config_json = read_file(config_path);
    std::istringstream ss(config_json);
    boost::property_tree::read_json(ss, config);
  }
  date::set_install(tzdata_path);

  run_http_server(config, port, num_workers);

  return 0;
}
