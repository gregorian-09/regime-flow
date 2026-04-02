#include <regimeflow/common/config.h>

int main() {
    regimeflow::Config config;
    config.set("name", "smoke");
    const auto value = config.get_as<std::string>("name");
    return value && *value == "smoke" ? 0 : 1;
}
