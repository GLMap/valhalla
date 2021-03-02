#include <string>
#include <vector>
#include <functional>

extern "C" {
namespace valhalla {
void Execute(const std::string &valhallaConfig, const std::vector<std::string> &tars,
             const std::string &json, bool optimize, const std::function<void()> &interrupt, std::string &result);

}
}