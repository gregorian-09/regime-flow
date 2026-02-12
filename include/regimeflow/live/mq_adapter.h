/**
 * @file mq_adapter.h
 * @brief RegimeFlow regimeflow mq adapter declarations.
 */

#pragma once

#include "regimeflow/common/result.h"
#include "regimeflow/live/event_bus.h"

#include <functional>
#include <memory>
#include <string>

namespace regimeflow::live {

/**
 * @brief Configuration for message queue integrations.
 */
struct MessageQueueConfig {
    /**
     * @brief Queue type (zeromq | kafka | redis).
     */
    std::string type;               // zeromq | kafka | redis
    /**
     * @brief Publish endpoint or broker list.
     */
    std::string publish_endpoint;   // endpoint or broker list
    /**
     * @brief Subscribe endpoint or broker list.
     */
    std::string subscribe_endpoint; // endpoint or broker list
    /**
     * @brief Topic or stream name.
     */
    std::string topic = "regimeflow";
    /**
     * @brief Poll timeout in milliseconds.
     */
    int poll_timeout_ms = 50;
    /**
     * @brief Redis stream name.
     */
    std::string redis_stream = "regimeflow";
    /**
     * @brief Redis consumer group.
     */
    std::string redis_group = "regimeflow-live";
    /**
     * @brief Redis consumer name (optional).
     */
    std::string redis_consumer = "";
    /**
     * @brief Initial reconnect backoff in milliseconds.
     */
    int64_t reconnect_initial_ms = 500;
    /**
     * @brief Maximum reconnect backoff in milliseconds.
     */
    int64_t reconnect_max_ms = 10'000;
    /**
     * @brief Maximum reconnect attempts (0 = unlimited).
     */
    int reconnect_max_attempts = 0;
};

/**
 * @brief Abstract message queue adapter.
 */
class MessageQueueAdapter {
public:
    virtual ~MessageQueueAdapter() = default;

    /**
     * @brief Connect to the queue.
     */
    virtual Result<void> connect() = 0;
    /**
     * @brief Disconnect from the queue.
     */
    virtual Result<void> disconnect() = 0;
    /**
     * @brief Check connection status.
     */
    virtual bool is_connected() const = 0;

    /**
     * @brief Publish a live message.
     * @param message Message to publish.
     */
    virtual Result<void> publish(const LiveMessage& message) = 0;
    /**
     * @brief Register inbound message callback.
     */
    virtual void on_message(std::function<void(const LiveMessage&)> cb) = 0;
    /**
     * @brief Poll for inbound messages.
     */
    virtual void poll() = 0;
};

/**
 * @brief Factory for message queue adapters.
 * @param config Queue configuration.
 * @return Adapter instance.
 */
std::unique_ptr<MessageQueueAdapter> create_message_queue_adapter(const MessageQueueConfig& config);

}  // namespace regimeflow::live
