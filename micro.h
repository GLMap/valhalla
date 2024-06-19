#include <functional>
#include <string>
#include <vector>

extern "C" {
namespace valhalla {

#if !USE_OS_TZDB
void SetTZDataPath(const std::string& path);
#endif

void GetApproachAlert(const std::string& locale,
                      bool imperial,
                      double distance,
                      const std::string& instruction,
                      std::string& result);

bool Execute(const std::string& action,
             const std::string& json_query,
             const std::string& config,
             const std::vector<std::function<int(void)>>& tars,
             const std::function<void()>* interrupt,
             std::string& result);

void GetTimeRange(const std::string& condition, std::vector<uint64_t>& result);

} // namespace valhalla
}
