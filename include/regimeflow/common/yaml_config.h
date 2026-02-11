#pragma once

#include "regimeflow/common/config.h"

#include <string>

namespace regimeflow {

class YamlConfigLoader {
public:
    static Config load_file(const std::string& path);
};

}  // namespace regimeflow
