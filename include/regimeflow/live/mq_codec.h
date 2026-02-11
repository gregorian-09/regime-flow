#pragma once

#include "regimeflow/live/event_bus.h"

#include <optional>
#include <string>

namespace regimeflow::live {

class LiveMessageCodec {
public:
    static std::string encode(const LiveMessage& message);
    static std::optional<LiveMessage> decode(const std::string& payload);
};

}  // namespace regimeflow::live
