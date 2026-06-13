#include "regimeflow/engine/replay_journal.h"

#include "regimeflow/common/json.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <sstream>

namespace regimeflow::engine
{
    namespace {

        using common::JsonValue;

        std::string escape_json(const std::string_view value) {
            std::string out;
            out.reserve(value.size() + 8);
            for (const char ch : value) {
                switch (ch) {
                case '\\': out += "\\\\"; break;
                case '"': out += "\\\""; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default: out += ch; break;
                }
            }
            return out;
        }

        std::string event_type_name(const events::EventType type) {
            switch (type) {
            case events::EventType::Market: return "market";
            case events::EventType::Order: return "order";
            case events::EventType::System: return "system";
            case events::EventType::User: return "user";
            }
            return "user";
        }

        std::string market_kind_name(const events::MarketEventKind kind) {
            switch (kind) {
            case events::MarketEventKind::Bar: return "bar";
            case events::MarketEventKind::Tick: return "tick";
            case events::MarketEventKind::Quote: return "quote";
            case events::MarketEventKind::Book: return "book";
            }
            return "bar";
        }

        std::string order_kind_name(const events::OrderEventKind kind) {
            switch (kind) {
            case events::OrderEventKind::NewOrder: return "new_order";
            case events::OrderEventKind::Fill: return "fill";
            case events::OrderEventKind::Cancel: return "cancel";
            case events::OrderEventKind::Reject: return "reject";
            case events::OrderEventKind::Update: return "update";
            }
            return "update";
        }

        std::string system_kind_name(const events::SystemEventKind kind) {
            switch (kind) {
            case events::SystemEventKind::DayStart: return "day_start";
            case events::SystemEventKind::EndOfDay: return "end_of_day";
            case events::SystemEventKind::Timer: return "timer";
            case events::SystemEventKind::RegimeChange: return "regime_change";
            case events::SystemEventKind::TradingHalt: return "trading_halt";
            case events::SystemEventKind::TradingResume: return "trading_resume";
            }
            return "timer";
        }

        template<typename Enum>
        Result<Enum> invalid_enum(const std::string_view kind) {
            Error error(Error::Code::ParseError, "unknown replay event kind");
            error.details = std::string(kind);
            return Result<Enum>(std::move(error));
        }

        Result<events::EventType> parse_event_type(const std::string_view value) {
            if (value == "market") return Ok(events::EventType::Market);
            if (value == "order") return Ok(events::EventType::Order);
            if (value == "system") return Ok(events::EventType::System);
            if (value == "user") return Ok(events::EventType::User);
            return invalid_enum<events::EventType>(value);
        }

        Result<events::MarketEventKind> parse_market_kind(const std::string_view value) {
            if (value == "bar") return Ok(events::MarketEventKind::Bar);
            if (value == "tick") return Ok(events::MarketEventKind::Tick);
            if (value == "quote") return Ok(events::MarketEventKind::Quote);
            if (value == "book") return Ok(events::MarketEventKind::Book);
            return invalid_enum<events::MarketEventKind>(value);
        }

        Result<events::OrderEventKind> parse_order_kind(const std::string_view value) {
            if (value == "new_order") return Ok(events::OrderEventKind::NewOrder);
            if (value == "fill") return Ok(events::OrderEventKind::Fill);
            if (value == "cancel") return Ok(events::OrderEventKind::Cancel);
            if (value == "reject") return Ok(events::OrderEventKind::Reject);
            if (value == "update") return Ok(events::OrderEventKind::Update);
            return invalid_enum<events::OrderEventKind>(value);
        }

        Result<events::SystemEventKind> parse_system_kind(const std::string_view value) {
            if (value == "day_start") return Ok(events::SystemEventKind::DayStart);
            if (value == "end_of_day") return Ok(events::SystemEventKind::EndOfDay);
            if (value == "timer") return Ok(events::SystemEventKind::Timer);
            if (value == "regime_change") return Ok(events::SystemEventKind::RegimeChange);
            if (value == "trading_halt") return Ok(events::SystemEventKind::TradingHalt);
            if (value == "trading_resume") return Ok(events::SystemEventKind::TradingResume);
            return invalid_enum<events::SystemEventKind>(value);
        }

        void write_common(std::ostringstream& out, const events::Event& event) {
            out << "{\"schema\":\"regimeflow.replay.v1\""
                << ",\"timestamp_us\":" << event.timestamp.microseconds()
                << ",\"type\":\"" << event_type_name(event.type) << "\""
                << ",\"priority\":" << static_cast<int>(event.priority)
                << ",\"sequence\":" << event.sequence
                << ",\"symbol\":" << event.symbol;
        }

        void write_book_levels(std::ostringstream& out, const std::array<data::BookLevel, 10>& levels) {
            out << '[';
            for (std::size_t i = 0; i < levels.size(); ++i) {
                if (i > 0) {
                    out << ',';
                }
                out << "[" << levels[i].price << ',' << levels[i].quantity << ',' << levels[i].num_orders << ']';
            }
            out << ']';
        }

        const JsonValue::Object* object_field(const JsonValue::Object& obj, const std::string& key) {
            const auto it = obj.find(key);
            if (it == obj.end()) {
                return nullptr;
            }
            return it->second.as_object();
        }

        Result<const JsonValue::Object*> required_object(const JsonValue& value) {
            if (const auto* obj = value.as_object()) {
                return Ok(obj);
            }
            return Result<const JsonValue::Object*>(Error(Error::Code::ParseError, "replay record must be a JSON object"));
        }

        Result<const JsonValue::Object*> required_object_field(const JsonValue::Object& obj, const std::string& key) {
            if (const auto* field = object_field(obj, key)) {
                return Ok(field);
            }
            Error error(Error::Code::ParseError, "missing replay object field");
            error.details = key;
            return Result<const JsonValue::Object*>(std::move(error));
        }

        Result<std::string> required_string(const JsonValue::Object& obj, const std::string& key) {
            const auto it = obj.find(key);
            if (it != obj.end()) {
                if (const auto* value = it->second.as_string()) {
                    return Ok(*value);
                }
            }
            Error error(Error::Code::ParseError, "missing replay string field");
            error.details = key;
            return Result<std::string>(std::move(error));
        }

        Result<double> required_number(const JsonValue::Object& obj, const std::string& key) {
            const auto it = obj.find(key);
            if (it != obj.end()) {
                if (const auto* value = it->second.as_number()) {
                    return Ok(*value);
                }
            }
            Error error(Error::Code::ParseError, "missing replay numeric field");
            error.details = key;
            return Result<double>(std::move(error));
        }

        std::string optional_string(const JsonValue::Object& obj, const std::string& key) {
            const auto it = obj.find(key);
            if (it == obj.end()) {
                return {};
            }
            if (const auto* value = it->second.as_string()) {
                return *value;
            }
            return {};
        }

        double optional_number(const JsonValue::Object& obj, const std::string& key, const double fallback = 0.0) {
            const auto it = obj.find(key);
            if (it == obj.end()) {
                return fallback;
            }
            if (const auto* value = it->second.as_number()) {
                return *value;
            }
            return fallback;
        }

        bool optional_bool(const JsonValue::Object& obj, const std::string& key, const bool fallback = false) {
            const auto it = obj.find(key);
            if (it == obj.end()) {
                return fallback;
            }
            if (const auto* value = it->second.as_bool()) {
                return *value;
            }
            return fallback;
        }

        data::BookLevel parse_book_level(const JsonValue& value) {
            data::BookLevel level;
            const auto* arr = value.as_array();
            if (arr == nullptr || arr->size() < 3) {
                return level;
            }
            if (const auto* price = (*arr)[0].as_number()) {
                level.price = *price;
            }
            if (const auto* quantity = (*arr)[1].as_number()) {
                level.quantity = *quantity;
            }
            if (const auto* num_orders = (*arr)[2].as_number()) {
                level.num_orders = static_cast<int>(*num_orders);
            }
            return level;
        }

        void parse_book_levels(const JsonValue::Object& data_obj,
                               const std::string& key,
                               std::array<data::BookLevel, 10>& levels) {
            const auto it = data_obj.find(key);
            if (it == data_obj.end()) {
                return;
            }
            const auto* arr = it->second.as_array();
            if (arr == nullptr) {
                return;
            }
            const auto count = std::min(arr->size(), levels.size());
            for (std::size_t i = 0; i < count; ++i) {
                levels[i] = parse_book_level((*arr)[i]);
            }
        }

        Result<events::MarketEventPayload> parse_market_payload(const std::string& kind,
                                                                const JsonValue::Object& data_obj) {
            auto kind_res = parse_market_kind(kind);
            if (kind_res.is_err()) {
                return Result<events::MarketEventPayload>(kind_res.error());
            }
            const auto event_kind = kind_res.value();
            auto timestamp_res = required_number(data_obj, "timestamp_us");
            auto symbol_res = required_number(data_obj, "symbol");
            if (timestamp_res.is_err()) {
                return Result<events::MarketEventPayload>(timestamp_res.error());
            }
            if (symbol_res.is_err()) {
                return Result<events::MarketEventPayload>(symbol_res.error());
            }
            const auto timestamp = Timestamp(static_cast<int64_t>(timestamp_res.value()));
            const auto symbol = static_cast<SymbolId>(symbol_res.value());

            switch (event_kind) {
            case events::MarketEventKind::Bar: {
                auto open = required_number(data_obj, "open");
                auto high = required_number(data_obj, "high");
                auto low = required_number(data_obj, "low");
                auto close = required_number(data_obj, "close");
                if (open.is_err()) return Result<events::MarketEventPayload>(open.error());
                if (high.is_err()) return Result<events::MarketEventPayload>(high.error());
                if (low.is_err()) return Result<events::MarketEventPayload>(low.error());
                if (close.is_err()) return Result<events::MarketEventPayload>(close.error());
                data::Bar bar;
                bar.timestamp = timestamp;
                bar.symbol = symbol;
                bar.open = open.value();
                bar.high = high.value();
                bar.low = low.value();
                bar.close = close.value();
                bar.volume = static_cast<Volume>(optional_number(data_obj, "volume"));
                bar.trade_count = static_cast<Volume>(optional_number(data_obj, "trade_count"));
                bar.vwap = optional_number(data_obj, "vwap");
                return Ok(events::MarketEventPayload{event_kind, bar});
            }
            case events::MarketEventKind::Tick: {
                auto price = required_number(data_obj, "price");
                if (price.is_err()) return Result<events::MarketEventPayload>(price.error());
                data::Tick tick;
                tick.timestamp = timestamp;
                tick.symbol = symbol;
                tick.price = price.value();
                tick.quantity = optional_number(data_obj, "quantity");
                tick.flags = static_cast<std::uint8_t>(optional_number(data_obj, "flags"));
                return Ok(events::MarketEventPayload{event_kind, tick});
            }
            case events::MarketEventKind::Quote: {
                auto bid = required_number(data_obj, "bid");
                auto ask = required_number(data_obj, "ask");
                if (bid.is_err()) return Result<events::MarketEventPayload>(bid.error());
                if (ask.is_err()) return Result<events::MarketEventPayload>(ask.error());
                data::Quote quote;
                quote.timestamp = timestamp;
                quote.symbol = symbol;
                quote.bid = bid.value();
                quote.ask = ask.value();
                quote.bid_size = optional_number(data_obj, "bid_size");
                quote.ask_size = optional_number(data_obj, "ask_size");
                return Ok(events::MarketEventPayload{event_kind, quote});
            }
            case events::MarketEventKind::Book: {
                data::OrderBook book;
                book.timestamp = timestamp;
                book.symbol = symbol;
                parse_book_levels(data_obj, "bids", book.bids);
                parse_book_levels(data_obj, "asks", book.asks);
                return Ok(events::MarketEventPayload{event_kind, book});
            }
            }
            return Result<events::MarketEventPayload>(Error(Error::Code::ParseError, "unknown market replay payload"));
        }

    }  // namespace

    Result<std::string> serialize_replay_event(const events::Event& event) {
        std::ostringstream out;
        write_common(out, event);

        if (const auto* payload = std::get_if<events::MarketEventPayload>(&event.payload)) {
            out << ",\"kind\":\"" << market_kind_name(payload->kind) << "\",\"data\":";
            std::visit([&]<typename T0>(const T0& data) {
                using T = std::decay_t<T0>;
                if constexpr (std::is_same_v<T, data::Bar>) {
                    out << "{\"timestamp_us\":" << data.timestamp.microseconds()
                        << ",\"symbol\":" << data.symbol
                        << ",\"open\":" << data.open
                        << ",\"high\":" << data.high
                        << ",\"low\":" << data.low
                        << ",\"close\":" << data.close
                        << ",\"volume\":" << data.volume
                        << ",\"trade_count\":" << data.trade_count
                        << ",\"vwap\":" << data.vwap << '}';
                } else if constexpr (std::is_same_v<T, data::Tick>) {
                    out << "{\"timestamp_us\":" << data.timestamp.microseconds()
                        << ",\"symbol\":" << data.symbol
                        << ",\"price\":" << data.price
                        << ",\"quantity\":" << data.quantity
                        << ",\"flags\":" << static_cast<int>(data.flags) << '}';
                } else if constexpr (std::is_same_v<T, data::Quote>) {
                    out << "{\"timestamp_us\":" << data.timestamp.microseconds()
                        << ",\"symbol\":" << data.symbol
                        << ",\"bid\":" << data.bid
                        << ",\"ask\":" << data.ask
                        << ",\"bid_size\":" << data.bid_size
                        << ",\"ask_size\":" << data.ask_size << '}';
                } else if constexpr (std::is_same_v<T, data::OrderBook>) {
                    out << "{\"timestamp_us\":" << data.timestamp.microseconds()
                        << ",\"symbol\":" << data.symbol
                        << ",\"bids\":";
                    write_book_levels(out, data.bids);
                    out << ",\"asks\":";
                    write_book_levels(out, data.asks);
                    out << '}';
                }
            }, payload->data);
        } else if (const auto* payload = std::get_if<events::OrderEventPayload>(&event.payload)) {
            out << ",\"kind\":\"" << order_kind_name(payload->kind) << "\",\"data\":{"
                << "\"order_id\":" << payload->order_id
                << ",\"parent_order_id\":" << payload->parent_order_id
                << ",\"fill_id\":" << payload->fill_id
                << ",\"quantity\":" << payload->quantity
                << ",\"price\":" << payload->price
                << ",\"commission\":" << payload->commission
                << ",\"transaction_cost\":" << payload->transaction_cost
                << ",\"is_maker\":" << (payload->is_maker ? "true" : "false")
                << ",\"venue\":\"" << escape_json(payload->venue) << "\"}";
        } else if (const auto* payload = std::get_if<events::SystemEventPayload>(&event.payload)) {
            out << ",\"kind\":\"" << system_kind_name(payload->kind) << "\",\"data\":{"
                << "\"code\":" << payload->code
                << ",\"id\":\"" << escape_json(payload->id) << "\"}";
        } else {
            out << ",\"kind\":\"none\",\"data\":{}";
        }

        out << '}';
        return Ok(out.str());
    }

    Result<events::Event> parse_replay_event(const std::string_view line) {
        auto parsed = common::parse_json(line);
        if (parsed.is_err()) {
            return Result<events::Event>(parsed.error());
        }
        auto root_res = required_object(parsed.value());
        if (root_res.is_err()) {
            return Result<events::Event>(root_res.error());
        }
        const auto& root = *root_res.value();

        auto timestamp = required_number(root, "timestamp_us");
        auto priority = required_number(root, "priority");
        auto sequence = required_number(root, "sequence");
        auto symbol = required_number(root, "symbol");
        auto type = required_string(root, "type");
        auto kind = required_string(root, "kind");
        auto data_res = required_object_field(root, "data");
        if (timestamp.is_err()) return Result<events::Event>(timestamp.error());
        if (priority.is_err()) return Result<events::Event>(priority.error());
        if (sequence.is_err()) return Result<events::Event>(sequence.error());
        if (symbol.is_err()) return Result<events::Event>(symbol.error());
        if (type.is_err()) return Result<events::Event>(type.error());
        if (kind.is_err()) return Result<events::Event>(kind.error());
        if (data_res.is_err()) return Result<events::Event>(data_res.error());

        events::Event event;
        event.timestamp = Timestamp(static_cast<int64_t>(timestamp.value()));
        event.priority = static_cast<std::uint8_t>(priority.value());
        event.sequence = static_cast<std::uint64_t>(sequence.value());
        event.symbol = static_cast<SymbolId>(symbol.value());

        auto type_res = parse_event_type(type.value());
        if (type_res.is_err()) {
            return Result<events::Event>(type_res.error());
        }
        event.type = type_res.value();
        const auto& data_obj = *data_res.value();

        switch (event.type) {
        case events::EventType::Market: {
            auto payload = parse_market_payload(kind.value(), data_obj);
            if (payload.is_err()) {
                return Result<events::Event>(payload.error());
            }
            event.payload = payload.value();
            break;
        }
        case events::EventType::Order: {
            auto event_kind = parse_order_kind(kind.value());
            if (event_kind.is_err()) {
                return Result<events::Event>(event_kind.error());
            }
            event.payload = events::OrderEventPayload{event_kind.value(),
                                                      static_cast<events::OrderId>(optional_number(data_obj, "order_id")),
                                                      static_cast<events::OrderId>(optional_number(data_obj, "parent_order_id")),
                                                      static_cast<events::FillId>(optional_number(data_obj, "fill_id")),
                                                      optional_number(data_obj, "quantity"),
                                                      optional_number(data_obj, "price"),
                                                      optional_number(data_obj, "commission"),
                                                      optional_number(data_obj, "transaction_cost"),
                                                      optional_bool(data_obj, "is_maker"),
                                                      optional_string(data_obj, "venue")};
            break;
        }
        case events::EventType::System: {
            auto event_kind = parse_system_kind(kind.value());
            if (event_kind.is_err()) {
                return Result<events::Event>(event_kind.error());
            }
            event.payload = events::SystemEventPayload{event_kind.value(),
                                                       static_cast<int64_t>(optional_number(data_obj, "code")),
                                                       optional_string(data_obj, "id")};
            break;
        }
        case events::EventType::User:
            event.payload = std::monostate{};
            break;
        }
        return Ok(event);
    }

    Result<std::vector<events::Event>> read_replay_journal(const std::string& path) {
        std::ifstream input(path);
        if (!input.is_open()) {
            Error error(Error::Code::IoError, "failed to open replay journal");
            error.details = path;
            return Result<std::vector<events::Event>>(std::move(error));
        }

        std::vector<events::Event> events;
        std::string line;
        std::size_t line_number = 0;
        while (std::getline(input, line)) {
            ++line_number;
            if (line.empty()) {
                continue;
            }
            auto event = parse_replay_event(line);
            if (event.is_err()) {
                Error error = event.error();
                error.details = "line=" + std::to_string(line_number) + ";" + error.details.value_or("");
                return Result<std::vector<events::Event>>(std::move(error));
            }
            events.push_back(event.value());
        }
        return Ok(events);
    }

    ReplayJournalWriter::ReplayJournalWriter(std::string path) : path_(std::move(path)) {}

    ReplayJournalWriter::~ReplayJournalWriter() {
        if (stream_.is_open()) {
            stream_.flush();
        }
    }

    Result<void> ReplayJournalWriter::append(const events::Event& event) {
        if (!stream_.is_open()) {
            stream_.open(path_, std::ios::out | std::ios::app);
        }
        if (!stream_.is_open()) {
            Error error(Error::Code::IoError, "failed to open replay journal for append");
            error.details = path_;
            return Result<void>(std::move(error));
        }
        auto serialized = serialize_replay_event(event);
        if (serialized.is_err()) {
            return Result<void>(serialized.error());
        }
        stream_ << serialized.value() << '\n';
        if (!stream_) {
            Error error(Error::Code::IoError, "failed to append replay journal event");
            error.details = path_;
            return Result<void>(std::move(error));
        }
        stream_.flush();
        return Ok();
    }
}  // namespace regimeflow::engine
