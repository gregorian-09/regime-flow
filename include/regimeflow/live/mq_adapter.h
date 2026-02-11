#pragma once

#include "regimeflow/common/result.h"
#include "regimeflow/live/event_bus.h"

#include <functional>
#include <memory>
#include <string>

namespace regimeflow::live {

struct MessageQueueConfig {
    std::string type;               // zeromq | kafka | redis
    std::string publish_endpoint;   // endpoint or broker list
    std::string subscribe_endpoint; // endpoint or broker list
    std::string topic = "regimeflow";
    int poll_timeout_ms = 50;
    std::string redis_stream = "regimeflow";
    std::string redis_group = "regimeflow-live";
    std::string redis_consumer = "";
    int64_t reconnect_initial_ms = 500;
    int64_t reconnect_max_ms = 10'000;
    int reconnect_max_attempts = 0;
};

class MessageQueueAdapter {
public:
    virtual ~MessageQueueAdapter() = default;

    virtual Result<void> connect() = 0;
    virtual Result<void> disconnect() = 0;
    virtual bool is_connected() const = 0;

    virtual Result<void> publish(const LiveMessage& message) = 0;
    virtual void on_message(std::function<void(const LiveMessage&)> cb) = 0;
    virtual void poll() = 0;
};

std::unique_ptr<MessageQueueAdapter> create_message_queue_adapter(const MessageQueueConfig& config);

}  // namespace regimeflow::live
