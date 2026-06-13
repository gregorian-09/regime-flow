#include "regimeflow/engine/backtest_engine.h"
#include "regimeflow/engine/replay_journal.h"
#include "regimeflow/events/event.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <variant>

namespace {
    struct ReplaySummary {
        std::size_t total = 0;
        std::size_t market = 0;
        std::size_t orders = 0;
        std::size_t system = 0;
        std::size_t user = 0;
    };

    void print_usage() {
        std::cout << "Usage: regimeflow_replay_journal --input <journal.jsonl> "
                  << "[--initial-capital <amount>] [--currency <code>] [--summary-only]\n";
    }

    ReplaySummary summarize(const std::vector<regimeflow::events::Event>& events) {
        ReplaySummary summary;
        summary.total = events.size();
        for (const auto& event : events) {
            switch (event.type) {
            case regimeflow::events::EventType::Market:
                ++summary.market;
                break;
            case regimeflow::events::EventType::Order:
                ++summary.orders;
                break;
            case regimeflow::events::EventType::System:
                ++summary.system;
                break;
            case regimeflow::events::EventType::User:
                ++summary.user;
                break;
            }
        }
        return summary;
    }

    void print_summary(const ReplaySummary& summary) {
        std::cout << "events.total=" << summary.total << '\n'
                  << "events.market=" << summary.market << '\n'
                  << "events.order=" << summary.orders << '\n'
                  << "events.system=" << summary.system << '\n'
                  << "events.user=" << summary.user << '\n';
    }
}  // namespace

int main(int argc, char** argv) {
    std::string input;
    std::string currency = "USD";
    double initial_capital = 100000.0;
    bool summary_only = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if ((arg == "--input" || arg == "-i") && i + 1 < argc) {
            input = argv[++i];
        } else if (arg == "--initial-capital" && i + 1 < argc) {
            initial_capital = std::stod(argv[++i]);
        } else if (arg == "--currency" && i + 1 < argc) {
            currency = argv[++i];
        } else if (arg == "--summary-only") {
            summary_only = true;
        } else if (arg == "--help" || arg == "-h") {
            print_usage();
            return 0;
        } else {
            std::cerr << "Unknown or incomplete argument: " << arg << '\n';
            print_usage();
            return 2;
        }
    }

    if (input.empty()) {
        print_usage();
        return 2;
    }

    auto events_result = regimeflow::engine::read_replay_journal(input);
    if (events_result.is_err()) {
        std::cerr << events_result.error().to_string() << '\n';
        return 1;
    }

    const auto& events = events_result.value();
    const auto summary = summarize(events);

    if (!summary_only) {
        regimeflow::engine::BacktestEngine engine(initial_capital, currency);
        for (auto event : events) {
            engine.enqueue(std::move(event));
        }
        engine.run();
        const auto results = engine.results();
        const auto latest = results.latest_account_snapshot();
        const double final_equity = latest.has_value() ? latest->equity : initial_capital;
        std::cout << "portfolio.final_equity=" << final_equity << '\n'
                  << "portfolio.total_return=" << results.total_return << '\n'
                  << "portfolio.max_drawdown=" << results.max_drawdown << '\n';
    }

    print_summary(summary);
    return 0;
}
