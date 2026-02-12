/**
 * @file mq_codec.h
 * @brief RegimeFlow regimeflow mq codec declarations.
 */

#pragma once

#include "regimeflow/live/event_bus.h"

#include <optional>
#include <string>

namespace regimeflow::live {

/**
 * @brief Encoding/decoding helpers for LiveMessage payloads.
 */
class LiveMessageCodec {
public:
    /**
     * @brief Encode a LiveMessage into a string payload.
     * @param message Message to encode.
     * @return Encoded string.
     */
    static std::string encode(const LiveMessage& message);
    /**
     * @brief Decode a LiveMessage from a string payload.
     * @param payload Encoded payload.
     * @return Optional message if decoding succeeds.
     */
    static std::optional<LiveMessage> decode(const std::string& payload);
};

}  // namespace regimeflow::live
