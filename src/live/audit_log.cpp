#include "regimeflow/live/audit_log.h"
#include "regimeflow/live/secret_hygiene.h"

#include <sstream>

namespace regimeflow::live
{
    AuditLogger::AuditLogger(std::string path) : path_(std::move(path)) {}

    Result<void> AuditLogger::log(const AuditEvent& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        open_if_needed();
        if (!stream_.is_open()) {
            return Result<void>(Error(Error::Code::IoError, "Failed to open audit log"));
        }
        stream_ << event.timestamp.to_string() << "," << type_to_string(event.type) << ","
                << sanitize(event.details);
        for (const auto& [k, v] : event.metadata) {
            stream_ << "," << k << "=" << sanitize(v);
        }
        stream_ << "\n";
        stream_.flush();
        return Ok();
    }

    Result<void> AuditLogger::log_error(const std::string& error) {
        AuditEvent event;
        event.timestamp = Timestamp::now();
        event.type = AuditEvent::Type::Error;
        event.details = error;
        return log(event);
    }

    Result<void> AuditLogger::log_error(const Error& error,
                                        std::string source,
                                        std::map<std::string, std::string> metadata) {
        AuditEvent event;
        event.timestamp = Timestamp::now();
        event.type = AuditEvent::Type::Error;
        event.details = error.message;
        if (!source.empty()) {
            metadata.emplace("source", std::move(source));
        }
        metadata.emplace("code", error_code_to_string(error.code));
        metadata.emplace("location", std::string(error.location.file_name()) + ":" + std::to_string(error.location.line()));
        if (error.details.has_value() && !error.details->empty()) {
            metadata.emplace("details", *error.details);
        }
        event.metadata = std::move(metadata);
        return log(event);
    }

    Result<void> AuditLogger::log_regime_change(const regime::RegimeTransition& transition) {
        AuditEvent event;
        event.timestamp = transition.timestamp;
        event.type = AuditEvent::Type::RegimeChange;
        event.details = "Regime change";
        event.metadata["from"] = std::to_string(static_cast<int>(transition.from));
        event.metadata["to"] = std::to_string(static_cast<int>(transition.to));
        event.metadata["confidence"] = std::to_string(transition.confidence);
        return log(event);
    }

    void AuditLogger::open_if_needed() {
        if (stream_.is_open()) {
            return;
        }
        stream_.open(path_, std::ios::out | std::ios::app);
    }

    std::string AuditLogger::sanitize(const std::string& value) {
        return redact_sensitive_values(value);
    }

    std::string AuditLogger::error_code_to_string(const Error::Code code) {
        switch (code) {
        case Error::Code::Ok: return "ok";
        case Error::Code::InvalidArgument: return "invalid_argument";
        case Error::Code::NotFound: return "not_found";
        case Error::Code::AlreadyExists: return "already_exists";
        case Error::Code::OutOfRange: return "out_of_range";
        case Error::Code::InvalidState: return "invalid_state";
        case Error::Code::IoError: return "io";
        case Error::Code::ParseError: return "parse";
        case Error::Code::ConfigError: return "config";
        case Error::Code::PluginError: return "plugin";
        case Error::Code::BrokerError: return "broker";
        case Error::Code::NetworkError: return "network";
        case Error::Code::TimeoutError: return "timeout";
        case Error::Code::InternalError: return "internal";
        case Error::Code::Unknown: return "unknown";
        }
        return "unknown";
    }

    std::string AuditLogger::type_to_string(AuditEvent::Type type) {
        switch (type) {
        case AuditEvent::Type::OrderSubmitted: return "OrderSubmitted";
        case AuditEvent::Type::OrderAcknowledged: return "OrderAcknowledged";
        case AuditEvent::Type::OrderFilled: return "OrderFilled";
        case AuditEvent::Type::OrderCancelled: return "OrderCancelled";
        case AuditEvent::Type::OrderRejected: return "OrderRejected";
        case AuditEvent::Type::PositionOpened: return "PositionOpened";
        case AuditEvent::Type::PositionClosed: return "PositionClosed";
        case AuditEvent::Type::RegimeChange: return "RegimeChange";
        case AuditEvent::Type::RiskLimitBreached: return "RiskLimitBreached";
        case AuditEvent::Type::SystemStart: return "SystemStart";
        case AuditEvent::Type::SystemStop: return "SystemStop";
        case AuditEvent::Type::Error: return "Error";
        }
        return "Error";
    }
}  // namespace regimeflow::live
