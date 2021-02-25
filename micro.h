#include <string>
#include <variant>
#include <functional>

typedef struct Error{
    uint code;
    std::string what;
} Error;

std::variant<std::string, Error> computeRoute(std::string &&valhallaConfig, std::vector<std::string>&& tars, std::string &&json, bool optimize, const std::function<void()> &interrupt);
