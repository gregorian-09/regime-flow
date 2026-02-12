/**
 * @file audit_log.h
 * @brief RegimeFlow regimeflow audit log declarations.
 */

#pragma once

#include "regimeflow/common/result.h"
#include "regimeflow/common/types.h"
#include "regimeflow/regime/types.h"

#include <fstream>
#include <map>
#include <mutex>
#include <string>

namespace regimeflow::live {

/**
 * @brief Structured audit log event for live trading.
 */
struct AuditEvent {
    /**
     * @brief Audit event type.
     */
    enum class Type {
        OrderSubmitted,
        OrderAcknowledged,
        OrderFilled,
        OrderCancelled,
        OrderRejected,
        PositionOpened,
        PositionClosed,
        RegimeChange,
        RiskLimitBreached,
        SystemStart,
        SystemStop,
        Error
    };

    Timestamp timestamp;
    Type type = Type::Error;
    std::string details;
    std::map<std::string, std::string> metadata;
};

/**
 * @brief Thread-safe audit logger for live trading.
 */
class AuditLogger {
public:
    /**
     * @brief Construct an audit logger with an output path.
     * @param path File path to write audit events.
     */
    explicit AuditLogger(std::string path);

    /**
     * @brief Log an audit event.
     * @param event Event to write.
     * @return Ok on success, IoError on failure.
     */
    Result<void> log(const AuditEvent& event);
    /**
     * @brief Log an error event.
     * @param error Error message.
     * @return Ok on success, IoError on failure.
     */
    Result<void> log_error(const std::string& error);
    /**
     * @brief Log a regime transition.
     * @param transition Regime transition data.
     * @return Ok on success, IoError on failure.
     */
    Result<void> log_regime_change(const regime::RegimeTransition& transition);

private:
    std::string path_;
    std::mutex mutex_;
    std::ofstream stream_;

    void open_if_needed();
    static std::string type_to_string(AuditEvent::Type type);
};

}  // namespace regimeflow::live
