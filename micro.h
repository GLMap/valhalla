#include <string>
#include <vector>
#include <functional>

extern "C" {
namespace valhalla {

#if !USE_OS_TZDB
void SetTZDataPath(const std::string &path);
#endif

void GetApproachAlert(const std::string& locale, bool imperial, double distance, const std::string& instruction, std::string &result);

void Execute(const std::string &valhallaConfig, const std::vector<std::string> &tars,
             const std::string &json, bool optimize, const std::function<void()> &interrupt, std::string &result);

}
}
