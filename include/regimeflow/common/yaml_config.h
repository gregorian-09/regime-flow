/**
 * @file yaml_config.h
 * @brief RegimeFlow regimeflow yaml config declarations.
 */

#pragma once

#include "regimeflow/common/config.h"

#include <string>

namespace regimeflow {

/**
 * @brief YAML configuration loader.
 */
class YamlConfigLoader {
public:
    /**
     * @brief Load a config from a YAML file.
     * @param path Path to YAML file.
     * @return Parsed Config.
     */
    static Config load_file(const std::string& path);
};

}  // namespace regimeflow
