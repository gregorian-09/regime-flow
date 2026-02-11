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

enum class LiveTopic : uint8_t {
    MarketData,
    ExecutionReport,
    PositionUpdate,
    AccountUpdate,
    System
};

struct LiveMessage {
    LiveTopic topic = LiveTopic::System;
    std::variant<std::monostate, MarketDataUpdate, ExecutionReport, Position, AccountInfo, std::string> payload;
    std::string origin;
};

class EventBus {
public:
    using SubscriptionId = uint64_t;
    using Callback = std::function<void(const LiveMessage&)>;

    EventBus();
    ~EventBus();

    void start();
    void stop();

    SubscriptionId subscribe(LiveTopic topic, Callback callback);
    void unsubscribe(SubscriptionId id);

    void publish(LiveMessage message);

private:
    void dispatch_loop();

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
