#include <string>
#include <vector>
#include <functional>

extern "C" {
namespace valhalla {

#if !USE_OS_TZDB
void SetTZDataPath(const std::string &path);
#endif

typedef int (*FileOpenFunction)(const std::string &path);

void GetApproachAlert(const std::string& locale, bool imperial, double distance, const std::string& instruction, std::string &result);

void Execute(const std::string &valhallaConfig, const std::vector<std::string> &tars, FileOpenFunction fileOpenFunction,
             const std::string &json, bool optimize, const std::function<void()> &interrupt, std::string &result);

void GetTimeRange(const std::string& condition, std::vector<uint64_t> &result);

}
}
