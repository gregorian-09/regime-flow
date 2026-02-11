#include "regimeflow/live/mq_adapter.h"

#include "regimeflow/live/mq_codec.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <thread>

#if defined(REGIMEFLOW_USE_ZMQ)
#include <zmq.hpp>
#endif

#if defined(REGIMEFLOW_USE_REDIS)
#if defined(_WIN32)
#include <process.h>
#else
#include <poll.h>
#include <unistd.h>
#endif
#include <hiredis/hiredis.h>
#endif

#if defined(REGIMEFLOW_USE_KAFKA)
#include <librdkafka/rdkafka.h>
#endif

namespace regimeflow::live {

namespace {

std::pair<std::string, int> parse_host_port(const std::string& endpoint, int default_port) {
    auto pos = endpoint.find(':');
    if (pos == std::string::npos) {
        return {endpoint, default_port};
    }
    std::string host = endpoint.substr(0, pos);
    int port = std::stoi(endpoint.substr(pos + 1));
    return {host, port};
}

int get_process_id() {
#if defined(_WIN32)
    return _getpid();
#else
    return ::getpid();
#endif
}

}  // namespace

#if defined(REGIMEFLOW_USE_ZMQ)

class ZmqAdapter final : public MessageQueueAdapter {
public:
    explicit ZmqAdapter(MessageQueueConfig config)
        : config_(std::move(config)), context_(1), pub_(context_, ZMQ_PUB), sub_(context_, ZMQ_SUB) {}

    Result<void> connect() override {
        if (connected_) {
            return Ok();
        }
        if (!config_.publish_endpoint.empty()) {
            if (starts_with(config_.publish_endpoint, "connect:")) {
                pub_.connect(config_.publish_endpoint.substr(8));
            } else if (starts_with(config_.publish_endpoint, "bind:")) {
                pub_.bind(config_.publish_endpoint.substr(5));
            } else {
                pub_.bind(config_.publish_endpoint);
            }
        }
        if (!config_.subscribe_endpoint.empty()) {
            if (starts_with(config_.subscribe_endpoint, "bind:")) {
                sub_.bind(config_.subscribe_endpoint.substr(5));
            } else if (starts_with(config_.subscribe_endpoint, "connect:")) {
                sub_.connect(config_.subscribe_endpoint.substr(8));
            } else {
                sub_.connect(config_.subscribe_endpoint);
            }
        }
        std::string topic = config_.topic.empty() ? "regimeflow" : config_.topic;
        sub_.set(zmq::sockopt::subscribe, topic);
        connected_ = true;
        return Ok();
    }

    Result<void> disconnect() override {
        connected_ = false;
        return Ok();
    }

    bool is_connected() const override { return connected_; }

    Result<void> publish(const LiveMessage& message) override {
        if (!connected_) {
            return Error(Error::Code::InvalidState, "ZeroMQ adapter not connected");
        }
        std::string topic = config_.topic.empty() ? "regimeflow" : config_.topic;
        std::string payload = LiveMessageCodec::encode(message);
        zmq::message_t topic_frame(topic.data(), topic.size());
        zmq::message_t payload_frame(payload.data(), payload.size());
        pub_.send(topic_frame, zmq::send_flags::sndmore);
        pub_.send(payload_frame, zmq::send_flags::none);
        return Ok();
    }

    void on_message(std::function<void(const LiveMessage&)> cb) override {
        callback_ = std::move(cb);
    }

    void poll() override {
        if (!connected_) {
            return;
        }
        zmq::pollitem_t items[] = {{sub_, 0, ZMQ_POLLIN, 0}};
        zmq::poll(items, 1, std::chrono::milliseconds(config_.poll_timeout_ms));
        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t topic_frame;
            zmq::message_t payload_frame;
            if (!sub_.recv(topic_frame, zmq::recv_flags::none)) {
                return;
            }
            if (!sub_.recv(payload_frame, zmq::recv_flags::none)) {
                return;
            }
            std::string payload(static_cast<char*>(payload_frame.data()), payload_frame.size());
            auto decoded = LiveMessageCodec::decode(payload);
            if (decoded && callback_) {
                callback_(*decoded);
            }
        }
    }

private:
    static bool starts_with(const std::string& s, const char* prefix) {
        return s.rfind(prefix, 0) == 0;
    }

    MessageQueueConfig config_;
    zmq::context_t context_;
    zmq::socket_t pub_;
    zmq::socket_t sub_;
    std::function<void(const LiveMessage&)> callback_;
    std::atomic<bool> connected_{false};
};

#endif

#if defined(REGIMEFLOW_USE_REDIS)

class RedisAdapter final : public MessageQueueAdapter {
public:
    explicit RedisAdapter(MessageQueueConfig config) : config_(std::move(config)) {}

    Result<void> connect() override {
        if (connected_) {
            return Ok();
        }
        auto [host, port] = parse_host_port(
            config_.publish_endpoint.empty() ? "127.0.0.1:6379" : config_.publish_endpoint, 6379);
        pub_ctx_ = redisConnect(host.c_str(), port);
        if (!pub_ctx_ || pub_ctx_->err) {
            return Error(Error::Code::InvalidState, "Redis publish connection failed");
        }
        auto [sub_host, sub_port] = parse_host_port(
            config_.subscribe_endpoint.empty() ? host + ":" + std::to_string(port)
                                               : config_.subscribe_endpoint,
            port);
        sub_ctx_ = redisConnect(sub_host.c_str(), sub_port);
        if (!sub_ctx_ || sub_ctx_->err) {
            return Error(Error::Code::InvalidState, "Redis subscribe connection failed");
        }
        stream_name_ = config_.redis_stream.empty() ? config_.topic : config_.redis_stream;
        if (stream_name_.empty()) {
            stream_name_ = "regimeflow";
        }
        group_name_ = config_.redis_group.empty() ? "regimeflow-live" : config_.redis_group;
        consumer_name_ = config_.redis_consumer;
        if (consumer_name_.empty()) {
            consumer_name_ = "live-engine-" + std::to_string(get_process_id());
        }
        redisReply* reply = reinterpret_cast<redisReply*>(
            redisCommand(sub_ctx_, "XGROUP CREATE %s %s $ MKSTREAM",
                         stream_name_.c_str(), group_name_.c_str()));
        if (reply) {
            freeReplyObject(reply);
        }
        connected_ = true;
        return Ok();
    }

    Result<void> disconnect() override {
        connected_ = false;
        if (pub_ctx_) {
            redisFree(pub_ctx_);
            pub_ctx_ = nullptr;
        }
        if (sub_ctx_) {
            redisFree(sub_ctx_);
            sub_ctx_ = nullptr;
        }
        return Ok();
    }

    bool is_connected() const override { return connected_; }

    Result<void> publish(const LiveMessage& message) override {
        if (!connected_) {
            return Error(Error::Code::InvalidState, "Redis adapter not connected");
        }
        std::string payload = LiveMessageCodec::encode(message);
        redisReply* reply = reinterpret_cast<redisReply*>(
            redisCommand(pub_ctx_, "XADD %s * payload %s", stream_name_.c_str(), payload.c_str()));
        if (reply) {
            freeReplyObject(reply);
        }
        return Ok();
    }

    void on_message(std::function<void(const LiveMessage&)> cb) override {
        callback_ = std::move(cb);
    }

    void poll() override {
        if (!connected_) {
            attempt_reconnect();
            return;
        }
        if (!sub_ctx_) {
            connected_ = false;
            return;
        }
        redisReply* reply = reinterpret_cast<redisReply*>(
            redisCommand(sub_ctx_, "XREADGROUP GROUP %s %s COUNT 10 BLOCK %d STREAMS %s >",
                         group_name_.c_str(),
                         consumer_name_.c_str(),
                         config_.poll_timeout_ms,
                         stream_name_.c_str()));
        if (!reply) {
            connected_ = false;
            return;
        }
        if (reply->type == REDIS_REPLY_NIL) {
            freeReplyObject(reply);
            return;
        }
        if (reply->type == REDIS_REPLY_ARRAY) {
            for (size_t i = 0; i < reply->elements; ++i) {
                redisReply* stream = reply->element[i];
                if (!stream || stream->elements < 2) {
                    continue;
                }
                redisReply* entries = stream->element[1];
                if (!entries || entries->type != REDIS_REPLY_ARRAY) {
                    continue;
                }
                for (size_t j = 0; j < entries->elements; ++j) {
                    redisReply* entry = entries->element[j];
                    if (!entry || entry->elements < 2) {
                        continue;
                    }
                    std::string entry_id(entry->element[0]->str, entry->element[0]->len);
                    redisReply* fields = entry->element[1];
                    std::string payload;
                    if (fields && fields->type == REDIS_REPLY_ARRAY) {
                        for (size_t k = 0; k + 1 < fields->elements; k += 2) {
                            std::string key(fields->element[k]->str, fields->element[k]->len);
                            if (key == "payload") {
                                payload.assign(fields->element[k + 1]->str,
                                               fields->element[k + 1]->len);
                                break;
                            }
                        }
                    }
                    if (!payload.empty()) {
                        auto decoded = LiveMessageCodec::decode(payload);
                        if (decoded && callback_) {
                            callback_(*decoded);
                        }
                    }
                    redisReply* ack = reinterpret_cast<redisReply*>(
                        redisCommand(sub_ctx_, "XACK %s %s %s",
                                     stream_name_.c_str(), group_name_.c_str(), entry_id.c_str()));
                    if (ack) {
                        freeReplyObject(ack);
                    }
                }
            }
        }
        freeReplyObject(reply);
    }

private:
    void attempt_reconnect() {
        if (config_.reconnect_max_attempts > 0 &&
            reconnect_attempts_ >= config_.reconnect_max_attempts) {
            return;
        }
        auto now = std::chrono::steady_clock::now();
        if (now < next_reconnect_) {
            return;
        }
        disconnect();
        auto res = connect();
        if (res.is_ok()) {
            reconnect_attempts_ = 0;
            backoff_ms_ = 0;
            next_reconnect_ = std::chrono::steady_clock::now();
            return;
        }
        reconnect_attempts_ += 1;
        if (backoff_ms_ <= 0) {
            backoff_ms_ = config_.reconnect_initial_ms;
        } else {
            backoff_ms_ = std::min(backoff_ms_ * 2, config_.reconnect_max_ms);
        }
        if (backoff_ms_ <= 0) {
            backoff_ms_ = 500;
        }
        next_reconnect_ = now + std::chrono::milliseconds(backoff_ms_);
    }

    MessageQueueConfig config_;
    redisContext* pub_ctx_ = nullptr;
    redisContext* sub_ctx_ = nullptr;
    std::function<void(const LiveMessage&)> callback_;
    std::atomic<bool> connected_{false};
    std::string stream_name_;
    std::string group_name_;
    std::string consumer_name_;
    int reconnect_attempts_ = 0;
    int64_t backoff_ms_ = 0;
    std::chrono::steady_clock::time_point next_reconnect_{};
};

#endif

#if defined(REGIMEFLOW_USE_KAFKA)

class KafkaAdapter final : public MessageQueueAdapter {
public:
    explicit KafkaAdapter(MessageQueueConfig config) : config_(std::move(config)) {}

    Result<void> connect() override {
        if (connected_) {
            return Ok();
        }
        char errstr[512];
        rd_kafka_conf_t* conf = rd_kafka_conf_new();
        rd_kafka_conf_set(conf, "bootstrap.servers",
                          config_.publish_endpoint.c_str(), errstr, sizeof(errstr));
        rd_kafka_conf_set(conf, "enable.auto.commit", "true", errstr, sizeof(errstr));
        producer_ = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
        if (!producer_) {
            return Error(Error::Code::InvalidState, errstr);
        }

        rd_kafka_conf_t* cconf = rd_kafka_conf_new();
        rd_kafka_conf_set(cconf, "bootstrap.servers",
                          config_.subscribe_endpoint.c_str(), errstr, sizeof(errstr));
        rd_kafka_conf_set(cconf, "group.id", "regimeflow", errstr, sizeof(errstr));
        rd_kafka_conf_set(cconf, "auto.offset.reset", "latest", errstr, sizeof(errstr));
        consumer_ = rd_kafka_new(RD_KAFKA_CONSUMER, cconf, errstr, sizeof(errstr));
        if (!consumer_) {
            return Error(Error::Code::InvalidState, errstr);
        }
        rd_kafka_poll_set_consumer(consumer_);
        rd_kafka_topic_partition_list_t* topics = rd_kafka_topic_partition_list_new(1);
        std::string topic = config_.topic.empty() ? "regimeflow" : config_.topic;
        rd_kafka_topic_partition_list_add(topics, topic.c_str(), RD_KAFKA_PARTITION_UA);
        rd_kafka_subscribe(consumer_, topics);
        rd_kafka_topic_partition_list_destroy(topics);
        connected_ = true;
        return Ok();
    }

    Result<void> disconnect() override {
        connected_ = false;
        if (consumer_) {
            rd_kafka_consumer_close(consumer_);
            rd_kafka_destroy(consumer_);
            consumer_ = nullptr;
        }
        if (producer_) {
            rd_kafka_flush(producer_, 1000);
            rd_kafka_destroy(producer_);
            producer_ = nullptr;
        }
        return Ok();
    }

    bool is_connected() const override { return connected_; }

    Result<void> publish(const LiveMessage& message) override {
        if (!connected_ || !producer_) {
            return Error(Error::Code::InvalidState, "Kafka adapter not connected");
        }
        std::string payload = LiveMessageCodec::encode(message);
        std::string topic = config_.topic.empty() ? "regimeflow" : config_.topic;
        int res = rd_kafka_producev(
            producer_,
            RD_KAFKA_V_TOPIC(topic.c_str()),
            RD_KAFKA_V_PARTITION(RD_KAFKA_PARTITION_UA),
            RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
            RD_KAFKA_V_VALUE(const_cast<char*>(payload.data()), payload.size()),
            RD_KAFKA_V_END);
        if (res != 0) {
            connected_ = false;
            return Error(Error::Code::InvalidState, rd_kafka_err2str(rd_kafka_last_error()));
        }
        rd_kafka_poll(producer_, 0);
        return Ok();
    }

    void on_message(std::function<void(const LiveMessage&)> cb) override {
        callback_ = std::move(cb);
    }

    void poll() override {
        if (!connected_) {
            attempt_reconnect();
            return;
        }
        if (!consumer_) {
            connected_ = false;
            return;
        }
        rd_kafka_message_t* msg = rd_kafka_consumer_poll(consumer_, config_.poll_timeout_ms);
        if (!msg) {
            return;
        }
        if (msg->err == RD_KAFKA_RESP_ERR_NO_ERROR) {
            std::string payload(static_cast<char*>(msg->payload),
                                static_cast<size_t>(msg->len));
            auto decoded = LiveMessageCodec::decode(payload);
            if (decoded && callback_) {
                callback_(*decoded);
            }
        } else if (msg->err != RD_KAFKA_RESP_ERR__PARTITION_EOF) {
            connected_ = false;
        }
        rd_kafka_message_destroy(msg);
    }

private:
    void attempt_reconnect() {
        if (config_.reconnect_max_attempts > 0 &&
            reconnect_attempts_ >= config_.reconnect_max_attempts) {
            return;
        }
        auto now = std::chrono::steady_clock::now();
        if (now < next_reconnect_) {
            return;
        }
        disconnect();
        auto res = connect();
        if (res.is_ok()) {
            reconnect_attempts_ = 0;
            backoff_ms_ = 0;
            next_reconnect_ = std::chrono::steady_clock::now();
            return;
        }
        reconnect_attempts_ += 1;
        if (backoff_ms_ <= 0) {
            backoff_ms_ = config_.reconnect_initial_ms;
        } else {
            backoff_ms_ = std::min(backoff_ms_ * 2, config_.reconnect_max_ms);
        }
        if (backoff_ms_ <= 0) {
            backoff_ms_ = 500;
        }
        next_reconnect_ = now + std::chrono::milliseconds(backoff_ms_);
    }

    MessageQueueConfig config_;
    rd_kafka_t* producer_ = nullptr;
    rd_kafka_t* consumer_ = nullptr;
    std::function<void(const LiveMessage&)> callback_;
    std::atomic<bool> connected_{false};
    int reconnect_attempts_ = 0;
    int64_t backoff_ms_ = 0;
    std::chrono::steady_clock::time_point next_reconnect_{};
};

#endif

std::unique_ptr<MessageQueueAdapter> create_message_queue_adapter(
    const MessageQueueConfig& config) {
#if defined(REGIMEFLOW_USE_ZMQ)
    if (config.type == "zeromq") {
        return std::make_unique<ZmqAdapter>(config);
    }
#endif
#if defined(REGIMEFLOW_USE_REDIS)
    if (config.type == "redis") {
        return std::make_unique<RedisAdapter>(config);
    }
#endif
#if defined(REGIMEFLOW_USE_KAFKA)
    if (config.type == "kafka") {
        return std::make_unique<KafkaAdapter>(config);
    }
#endif
    return nullptr;
}

}  // namespace regimeflow::live
