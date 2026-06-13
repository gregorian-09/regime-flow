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

namespace regimeflow::live
{
    /**
     * @brief On-disk audit log encoding.
     */
    enum class AuditLogFormat {
        Csv,
        Jsonl
    };

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
            DryRunOrder,
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
        explicit AuditLogger(std::string path, AuditLogFormat format = AuditLogFormat::Csv);

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
         * @brief Log a structured error event.
         * @param error Structured error object.
         * @param source Source subsystem or adapter.
         * @param metadata Additional metadata to attach.
         * @return Ok on success, IoError on failure.
         */
        Result<void> log_error(const Error& error,
                               std::string source = {},
                               std::map<std::string, std::string> metadata = {});
        /**
         * @brief Log a regime transition.
         * @param transition Regime transition data.
         * @return Ok on success, IoError on failure.
         */
        Result<void> log_regime_change(const regime::RegimeTransition& transition);
        /**
         * @brief Log a regime transition with model governance metadata.
         */
        Result<void> log_regime_change(const regime::RegimeTransition& transition,
                                       std::map<std::string, std::string> metadata);

    private:
        std::string path_;
        std::mutex mutex_;
        std::ofstream stream_;

        void open_if_needed();
        static std::string sanitize(const std::string& value);
        static std::string escape_json(const std::string& value);
        static std::string error_code_to_string(Error::Code code);
        static std::string type_to_string(AuditEvent::Type type);
        void write_csv(const AuditEvent& event);
        void write_jsonl(const AuditEvent& event);
        AuditLogFormat format_ = AuditLogFormat::Csv;
    };
}  // namespace regimeflow::live
