/**
 * @file event_bus.h
 * @brief RegimeFlow regimeflow event bus declarations.
 */

#pragma once

#include "regimeflow/live/broker_adapter.h"
#include "regimeflow/live/types.h"
#include "regimeflow/common/memory.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <unordered_map>

namespace regimeflow::live {

/**
 * @brief Topics for live event bus messages.
 */
enum class LiveTopic : uint8_t {
    MarketData,
    ExecutionReport,
    PositionUpdate,
    AccountUpdate,
    System
};

/**
 * @brief Live bus message wrapper.
 */
struct LiveMessage {
    LiveTopic topic = LiveTopic::System;
    std::variant<std::monostate, MarketDataUpdate, ExecutionReport, Position, AccountInfo, std::string> payload;
    std::string origin;
};

/**
 * @brief In-process event bus for live trading messages.
 */
class EventBus {
public:
    /**
     * @brief Subscription identifier.
     */
    using SubscriptionId = uint64_t;
    /**
     * @brief Subscriber callback.
     */
    using Callback = std::function<void(const LiveMessage&)>;

    /**
     * @brief Construct the event bus.
     */
    EventBus();
    /**
     * @brief Stop the bus on destruction.
     */
    ~EventBus();

    /**
     * @brief Start the dispatch loop.
     */
    void start();
    /**
     * @brief Stop the dispatch loop.
     */
    void stop();

    /**
     * @brief Subscribe to a topic.
     * @param topic Topic to subscribe to.
     * @param callback Callback for messages.
     * @return Subscription ID.
     */
    SubscriptionId subscribe(LiveTopic topic, Callback callback);
    /**
     * @brief Unsubscribe by ID.
     * @param id Subscription ID.
     */
    void unsubscribe(SubscriptionId id);

    /**
     * @brief Publish a message to the bus.
     * @param message Message to publish.
     */
    void publish(LiveMessage message);

private:
    void dispatch_loop();

    /**
     * @brief Internal node used for the pending list.
     */
    struct Node {
        LiveMessage message;
        Node* next = nullptr;
    };

    void drain_pending();

    std::atomic<bool> running_{false};
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::queue<LiveMessage> queue_;
    std::atomic<Node*> pending_{nullptr};
    common::PoolAllocator<Node> pool_{1024};

    std::mutex sub_mutex_;
    SubscriptionId next_id_ = 1;
    std::unordered_map<SubscriptionId, std::pair<LiveTopic, Callback>> subscribers_;
    std::thread dispatcher_;
};

}  // namespace regimeflow::live
