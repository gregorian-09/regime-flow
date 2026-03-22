#include "regimeflow/engine/backtest_results.h"

#include <sstream>

namespace regimeflow::engine
{
    namespace {
        std::string classify_category(const AuditEvent& event) {
            switch (event.type) {
            case AuditEvent::Type::OrderSubmitted:
            case AuditEvent::Type::OrderCancelled:
            case AuditEvent::Type::OrderRejected:
                return "trade";
            case AuditEvent::Type::OrderFilled:
                return "execution";
            case AuditEvent::Type::AccountUpdate:
                if (event.details.find("Margin") != std::string::npos
                    || event.details.find("Stop-out") != std::string::npos
                    || event.details.find("Trading halted") != std::string::npos) {
                    return "risk";
                }
                return "account";
            case AuditEvent::Type::RegimeChange:
                return "regime";
            case AuditEvent::Type::SystemStart:
            case AuditEvent::Type::SystemStop:
                return "system";
            case AuditEvent::Type::Error:
                return "error";
            }
            return "system";
        }

        std::string event_title(const AuditEvent& event) {
            switch (event.type) {
            case AuditEvent::Type::OrderSubmitted:
                return "Order Submitted";
            case AuditEvent::Type::OrderFilled:
                return "Order Filled";
            case AuditEvent::Type::OrderCancelled:
                return "Order Cancelled";
            case AuditEvent::Type::OrderRejected:
                return "Order Rejected";
            case AuditEvent::Type::AccountUpdate:
                return "Account Update";
            case AuditEvent::Type::RegimeChange:
                return "Regime Change";
            case AuditEvent::Type::SystemStart:
                return "Test Started";
            case AuditEvent::Type::SystemStop:
                return "Test Finished";
            case AuditEvent::Type::Error:
                return "Error";
            }
            return "Event";
        }

        std::string event_source(const AuditEvent& event) {
            if (const auto it = event.metadata.find("symbol"); it != event.metadata.end()) {
                return it->second;
            }
            return "tester";
        }

        std::string event_detail(const AuditEvent& event) {
            if (event.metadata.empty()) {
                return event.details;
            }
            std::ostringstream out;
            out << event.details << " [";
            bool first = true;
            for (const auto& [key, value] : event.metadata) {
                if (!first) {
                    out << ", ";
                }
                first = false;
                out << key << "=" << value;
            }
            out << "]";
            return out.str();
        }
    }  // namespace

    std::vector<TesterJournalEntry> BacktestResults::tester_journal() const {
        std::vector<TesterJournalEntry> rows;
        rows.reserve(journal_events.size());
        for (const auto& event : journal_events) {
            TesterJournalEntry row;
            row.timestamp = event.timestamp;
            row.source = event_source(event);
            row.category = classify_category(event);
            row.title = event_title(event);
            row.detail = event_detail(event);
            rows.emplace_back(std::move(row));
        }
        return rows;
    }
}  // namespace regimeflow::engine
