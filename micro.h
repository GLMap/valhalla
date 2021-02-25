#include <string>
#include <functional>

extern "C" {
namespace valhalla {

class Error : public std::exception {
public:
    unsigned int code;
    std::string what;
    Error(unsigned int code, const std::string &what):code(code), what(what) {}
};

void Execute(const std::string &valhallaConfig, const std::vector<std::string> &tars,
             const std::string &json, bool optimize, const std::function<void()> &interrupt, std::string &result);

}
}
