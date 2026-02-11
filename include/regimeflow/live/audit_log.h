#pragma once

#include "regimeflow/common/result.h"
#include "regimeflow/common/types.h"
#include "regimeflow/regime/types.h"

#include <fstream>
#include <map>
#include <mutex>
#include <string>

namespace regimeflow::live {

struct AuditEvent {
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

class AuditLogger {
public:
    explicit AuditLogger(std::string path);

    Result<void> log(const AuditEvent& event);
    Result<void> log_error(const std::string& error);
    Result<void> log_regime_change(const regime::RegimeTransition& transition);

private:
    std::string path_;
    std::mutex mutex_;
    std::ofstream stream_;

    void open_if_needed();
    static std::string type_to_string(AuditEvent::Type type);
};

}  // namespace regimeflow::live
