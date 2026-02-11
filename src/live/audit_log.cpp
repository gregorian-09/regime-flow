#include "regimeflow/live/audit_log.h"

#include <sstream>

namespace regimeflow::live {

AuditLogger::AuditLogger(std::string path) : path_(std::move(path)) {}

Result<void> AuditLogger::log(const AuditEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    open_if_needed();
    if (!stream_.is_open()) {
        return Error(Error::Code::IoError, "Failed to open audit log");
    }
    stream_ << event.timestamp.to_string() << "," << type_to_string(event.type) << ","
            << event.details;
    for (const auto& [k, v] : event.metadata) {
        stream_ << "," << k << "=" << v;
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
