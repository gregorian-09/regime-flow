#include "regimeflow/common/time.h"
#include "regimeflow/common/types.h"
#include "regimeflow/engine/backtest_engine.h"
#include "regimeflow/events/event.h"
#include "regimeflow/strategy/strategy.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

using namespace regimeflow;

namespace {
    class StrategyTesterDemoStrategy final : public strategy::Strategy {
    public:
        void initialize(strategy::StrategyContext& ctx) override {
            ctx_ = &ctx;
        }

        void on_quote(const data::Quote& quote) override {
            if (!entry_submitted_) {
                engine::Order order;
                order.symbol = quote.symbol;
                order.side = engine::OrderSide::Buy;
                order.type = engine::OrderType::Market;
                order.quantity = 10.0;
                order.tif = engine::TimeInForce::GTC;
                order.created_at = quote.timestamp;
                const auto result = ctx_->submit_order(order);
                if (result.is_ok()) {
                    entry_submitted_ = true;
                }
            }
        }

        void on_fill(const engine::Fill& fill) override {
            if (fill.quantity <= 0.0 || exit_submitted_) {
                return;
            }
            engine::Order order;
            order.symbol = fill.symbol;
            order.side = engine::OrderSide::Sell;
            order.type = engine::OrderType::Limit;
            order.quantity = fill.quantity;
            order.limit_price = fill.price + 1.0;
            order.tif = engine::TimeInForce::GTC;
            order.created_at = fill.timestamp;
            const auto result = ctx_->submit_order(order);
            if (result.is_ok()) {
                exit_submitted_ = true;
            }
        }

    private:
        strategy::StrategyContext* ctx_ = nullptr;
        bool entry_submitted_ = false;
        bool exit_submitted_ = false;
    };

    struct Options {
        bool json = false;
        bool live = true;
        bool ansi_colors = true;
        bool interactive_tabs = false;
        int sleep_ms = 125;
        std::string snapshot_file;
        engine::DashboardTab active_tab = engine::DashboardTab::All;
    };

    engine::DashboardTab parse_tab(const std::string& value) {
        if (value == "all") return engine::DashboardTab::All;
        if (value == "setup") return engine::DashboardTab::Setup;
        if (value == "report") return engine::DashboardTab::Report;
        if (value == "graph") return engine::DashboardTab::Graph;
        if (value == "trades") return engine::DashboardTab::Trades;
        if (value == "orders") return engine::DashboardTab::Orders;
        if (value == "venues") return engine::DashboardTab::Venues;
        if (value == "optimization") return engine::DashboardTab::Optimization;
        if (value == "journal") return engine::DashboardTab::Journal;
        return engine::DashboardTab::All;
    }

    std::string tab_name(const engine::DashboardTab tab) {
        switch (tab) {
        case engine::DashboardTab::All:
            return "all";
        case engine::DashboardTab::Setup:
            return "setup";
        case engine::DashboardTab::Report:
            return "report";
        case engine::DashboardTab::Graph:
            return "graph";
        case engine::DashboardTab::Trades:
            return "trades";
        case engine::DashboardTab::Orders:
            return "orders";
        case engine::DashboardTab::Venues:
            return "venues";
        case engine::DashboardTab::Optimization:
            return "optimization";
        case engine::DashboardTab::Journal:
            return "journal";
        }
        return "all";
    }

    Options parse_args(const int argc, char** argv) {
        Options options;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--json") {
                options.json = true;
                options.live = false;
            } else if (arg == "--no-live") {
                options.live = false;
            } else if (arg == "--no-ansi") {
                options.ansi_colors = false;
            } else if (arg == "--interactive-tabs" || arg == "--tui") {
                options.interactive_tabs = true;
                options.live = true;
            } else if (arg.rfind("--sleep-ms=", 0) == 0) {
                options.sleep_ms = std::max(0, std::atoi(arg.substr(11).c_str()));
            } else if (arg.rfind("--snapshot-file=", 0) == 0) {
                options.snapshot_file = arg.substr(16);
            } else if (arg.rfind("--tab=", 0) == 0) {
                options.active_tab = parse_tab(arg.substr(6));
            } else if (arg == "--help" || arg == "-h") {
                std::cout
                    << "Usage: regimeflow_strategy_tester [--json] [--no-live] [--no-ansi]"
                    << " [--interactive-tabs|--tui]"
                    << " [--tab=all|setup|report|graph|trades|orders|venues|optimization|journal]"
                    << " [--snapshot-file=PATH] [--sleep-ms=N]\n";
                std::exit(0);
            }
        }
        return options;
    }

    void enqueue_demo_feed(engine::BacktestEngine& engine, const SymbolId symbol) {
        data::Quote quote_one;
        quote_one.symbol = symbol;
        quote_one.bid = 100.0;
        quote_one.ask = 100.1;
        quote_one.timestamp = Timestamp::from_string("2020-01-02 09:30:00", "%Y-%m-%d %H:%M:%S");
        engine.enqueue(events::make_market_event(quote_one));

        data::Quote quote_two = quote_one;
        quote_two.bid = 100.2;
        quote_two.ask = 100.3;
        quote_two.timestamp = Timestamp::from_string("2020-01-02 09:30:01", "%Y-%m-%d %H:%M:%S");
        engine.enqueue(events::make_market_event(quote_two));

        data::Quote quote_three = quote_one;
        quote_three.bid = 100.8;
        quote_three.ask = 100.9;
        quote_three.timestamp = Timestamp::from_string("2020-01-02 09:30:02", "%Y-%m-%d %H:%M:%S");
        engine.enqueue(events::make_market_event(quote_three));

        data::Quote quote_four = quote_one;
        quote_four.bid = 101.2;
        quote_four.ask = 101.3;
        quote_four.timestamp = Timestamp::from_string("2020-01-02 09:30:03", "%Y-%m-%d %H:%M:%S");
        engine.enqueue(events::make_market_event(quote_four));
    }

    bool apply_tab_command(const std::string& command, engine::DashboardTab& active_tab) {
        if (command.empty()) {
            return true;
        }
        if (command == "q" || command == "quit" || command == "exit") {
            return false;
        }
        if (command == "n" || command == "next" || command == "]") {
            const auto next = static_cast<int>(active_tab) + 1;
            active_tab = next > static_cast<int>(engine::DashboardTab::Journal)
                ? engine::DashboardTab::All
                : static_cast<engine::DashboardTab>(next);
            return true;
        }
        if (command == "p" || command == "prev" || command == "[") {
            const auto previous = static_cast<int>(active_tab) - 1;
            active_tab = previous < static_cast<int>(engine::DashboardTab::All)
                ? engine::DashboardTab::Journal
                : static_cast<engine::DashboardTab>(previous);
            return true;
        }
        if (command.size() == 1 && command[0] >= '0' && command[0] <= '8') {
            const auto index = command[0] - '0';
            active_tab = static_cast<engine::DashboardTab>(index);
            return true;
        }
        active_tab = parse_tab(command);
        return true;
    }

    void write_snapshot_file(const engine::BacktestEngine& engine, const std::string& path) {
        if (path.empty()) {
            return;
        }
        std::ofstream out(path, std::ios::out | std::ios::trunc);
        if (!out) {
            return;
        }
        out << engine.dashboard_snapshot_json() << '\n';
    }

    bool stdin_is_tty() {
#ifdef _WIN32
        return _isatty(_fileno(stdin)) != 0;
#else
        return isatty(STDIN_FILENO) != 0;
#endif
    }

    int terminal_width() {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info) != 0) {
            return std::max<int>(80, info.srWindow.Right - info.srWindow.Left + 1);
        }
        return 120;
#else
        if (!stdin_is_tty()) {
            return 120;
        }
        winsize ws{};
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
            return std::max<int>(80, ws.ws_col);
        }
        return 120;
#endif
    }

    std::vector<std::string> split_lines(const std::string& text) {
        std::vector<std::string> lines;
        std::istringstream in(text);
        std::string line;
        while (std::getline(in, line)) {
            lines.emplace_back(std::move(line));
        }
        return lines;
    }

    std::vector<std::string> panel_lines(const engine::BacktestEngine& engine,
                                         const engine::DashboardRenderOptions& options) {
        auto lines = split_lines(engine.dashboard_terminal(engine::DashboardRenderOptions{
            false,
            options.max_rows,
            options.active_tab,
        }));
        if (lines.size() > 5) {
            lines.erase(lines.begin(), lines.begin() + 5);
        }
        return lines;
    }

    std::string truncate_to_width(const std::string& value, const size_t width) {
        if (value.size() <= width) {
            return value;
        }
        if (width <= 3) {
            return value.substr(0, width);
        }
        return value.substr(0, width - 3) + "...";
    }

    std::string pad_right(const std::string& value, const size_t width) {
        const auto truncated = truncate_to_width(value, width);
        if (truncated.size() >= width) {
            return truncated;
        }
        return truncated + std::string(width - truncated.size(), ' ');
    }

    engine::DashboardTab companion_tab(const engine::DashboardTab active_tab) {
        switch (active_tab) {
        case engine::DashboardTab::All:
        case engine::DashboardTab::Report:
            return engine::DashboardTab::Venues;
        case engine::DashboardTab::Setup:
            return engine::DashboardTab::Report;
        case engine::DashboardTab::Graph:
            return engine::DashboardTab::Trades;
        case engine::DashboardTab::Trades:
            return engine::DashboardTab::Orders;
        case engine::DashboardTab::Orders:
            return engine::DashboardTab::Journal;
        case engine::DashboardTab::Venues:
            return engine::DashboardTab::Report;
        case engine::DashboardTab::Optimization:
            return engine::DashboardTab::Report;
        case engine::DashboardTab::Journal:
            return engine::DashboardTab::Report;
        }
        return engine::DashboardTab::Report;
    }

    class TerminalInputController {
    public:
        TerminalInputController() {
            if (!stdin_is_tty()) {
                return;
            }
#ifdef _WIN32
            enabled_ = true;
#else
            if (tcgetattr(STDIN_FILENO, &original_) != 0) {
                return;
            }
            termios raw = original_;
            raw.c_lflag &= static_cast<unsigned long>(~(ICANON | ECHO));
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) {
                return;
            }
            enabled_ = true;
#endif
        }

        ~TerminalInputController() {
#ifndef _WIN32
            if (enabled_) {
                (void)tcsetattr(STDIN_FILENO, TCSANOW, &original_);
            }
#endif
        }

        bool enabled() const {
            return enabled_;
        }

        std::optional<std::string> poll_command() const {
            if (!enabled_) {
                return std::nullopt;
            }
#ifdef _WIN32
            if (_kbhit() == 0) {
                return std::nullopt;
            }
            const int ch = _getch();
            if (ch == 0 || ch == 224) {
                if (_kbhit() != 0) {
                    (void)_getch();
                }
                return std::nullopt;
            }
            const char c = static_cast<char>(ch);
            if (c == '\r' || c == '\n') {
                return std::nullopt;
            }
            return std::string(1, c);
#else
            fd_set read_set;
            FD_ZERO(&read_set);
            FD_SET(STDIN_FILENO, &read_set);
            timeval timeout{};
            timeout.tv_sec = 0;
            timeout.tv_usec = 0;
            const int ready = select(STDIN_FILENO + 1, &read_set, nullptr, nullptr, &timeout);
            if (ready <= 0 || FD_ISSET(STDIN_FILENO, &read_set) == 0) {
                return std::nullopt;
            }
            char c = 0;
            const auto bytes = ::read(STDIN_FILENO, &c, 1);
            if (bytes <= 0 || c == '\n' || c == '\r') {
                return std::nullopt;
            }
            if (c == '\033') {
                return std::nullopt;
            }
            return std::string(1, c);
#endif
        }

    private:
        bool enabled_ = false;
#ifndef _WIN32
        termios original_{};
#endif
    };

    void clear_screen(const bool ansi_colors) {
        if (ansi_colors) {
            std::cout << "\033[2J\033[H";
        } else {
            std::cout << "\n\n";
        }
    }

    void render_tui(const engine::BacktestEngine& engine,
                    const engine::DashboardRenderOptions& render_options,
                    const bool complete) {
        const auto snapshot = engine.dashboard_snapshot();
        const auto width = terminal_width();
        const auto panel_width = std::max(30, (width - 7) / 2);
        const auto left_tab = render_options.active_tab;
        const auto right_tab = companion_tab(left_tab);
        const auto left_lines = panel_lines(engine, render_options);
        const auto right_lines = panel_lines(engine, engine::DashboardRenderOptions{
            render_options.ansi_colors,
            render_options.max_rows,
            right_tab,
        });
        const auto rows = std::max(left_lines.size(), right_lines.size());

        clear_screen(render_options.ansi_colors);
        std::cout << "REGIMEFLOW STRATEGY TESTER TUI"
                  << " | active=" << tab_name(left_tab)
                  << " | state=" << (complete ? "complete" : "running")
                  << " | q quit | n/p cycle | 0-8 tabs\n";
        std::cout << "time=" << snapshot.timestamp.to_string("%Y-%m-%d %H:%M:%S")
                  << " equity=" << snapshot.equity
                  << " fills=" << snapshot.fill_count
                  << " open_orders=" << snapshot.open_order_count
                  << " strategy=" << (snapshot.setup.strategy_name.empty() ? "n/a" : snapshot.setup.strategy_name)
                  << "\n";
        std::cout << "+" << std::string(panel_width + 2, '-')
                  << "+ +" << std::string(panel_width + 2, '-') << "+\n";
        std::cout << "| " << pad_right("ACTIVE: " + tab_name(left_tab), static_cast<size_t>(panel_width))
                  << " | | " << pad_right("MONITOR: " + tab_name(right_tab), static_cast<size_t>(panel_width))
                  << " |\n";
        std::cout << "+" << std::string(panel_width + 2, '-')
                  << "+ +" << std::string(panel_width + 2, '-') << "+\n";
        for (size_t i = 0; i < rows; ++i) {
            const std::string left = i < left_lines.size() ? left_lines[i] : "";
            const std::string right = i < right_lines.size() ? right_lines[i] : "";
            std::cout << "| " << pad_right(left, static_cast<size_t>(panel_width))
                      << " | | " << pad_right(right, static_cast<size_t>(panel_width))
                      << " |\n";
        }
        std::cout << "+" << std::string(panel_width + 2, '-')
                  << "+ +" << std::string(panel_width + 2, '-') << "+\n";
        std::cout << "Snapshot JSON: " << (snapshot.setup.strategy_name.empty() ? "available" : snapshot.setup.strategy_name)
                  << " | setup symbols=" << (snapshot.setup.symbols.empty() ? 0 : snapshot.setup.symbols.size())
                  << "\n" << std::flush;
    }

    bool process_pending_input(const TerminalInputController* input,
                               engine::DashboardTab& active_tab) {
        if (input == nullptr || !input->enabled()) {
            return true;
        }
        while (true) {
            const auto command = input->poll_command();
            if (!command.has_value()) {
                return true;
            }
            if (!apply_tab_command(*command, active_tab)) {
                return false;
            }
        }
    }

    bool run_blocking_tab_prompt(const engine::BacktestEngine& engine,
                                 engine::DashboardRenderOptions& render_options,
                                 const std::string& snapshot_file) {
        std::string command;
        while (true) {
            std::cout
                << "\nTab command [0=all,1=setup,2=report,3=graph,4=trades,5=orders,6=venues,7=optimization,8=journal,n,p,q]: "
                << std::flush;
            if (!std::getline(std::cin, command)) {
                return true;
            }
            if (!apply_tab_command(command, render_options.active_tab)) {
                return false;
            }
            render_tui(engine, render_options, true);
            write_snapshot_file(engine, snapshot_file);
        }
    }
}  // namespace

int main(int argc, char** argv) {
    const auto options = parse_args(argc, argv);

    engine::BacktestEngine engine(100000.0, "USD");
    engine::DashboardSetup setup;
    setup.strategy_name = "strategy_tester_demo";
    setup.data_source = "manual_events";
    setup.symbols = {"DEMO"};
    setup.start_date = "2020-01-02";
    setup.end_date = "2020-01-02";
    setup.bar_type = "quote";
    setup.execution_model = "basic";
    setup.tick_mode = "real_ticks";
    setup.bar_mode = "close_only";
    engine.set_dashboard_setup(setup);

    Config execution;
    execution.set_path("model", "basic");
    execution.set_path("session.enabled", true);
    execution.set_path("session.start_hhmm", "09:30");
    execution.set_path("session.end_hhmm", "16:00");
    execution.set_path("simulation.tick_mode", "real_ticks");
    engine.configure_execution(execution);

    auto strategy = std::make_unique<StrategyTesterDemoStrategy>();
    engine.set_strategy(std::move(strategy));

    const auto symbol = SymbolRegistry::instance().intern("DEMO");
    enqueue_demo_feed(engine, symbol);

    engine::DashboardRenderOptions render_options{options.ansi_colors, 6, options.active_tab};
    const auto input = options.interactive_tabs
        ? std::make_unique<TerminalInputController>()
        : nullptr;

    bool continue_running = true;
    while (continue_running) {
        continue_running = process_pending_input(input.get(), render_options.active_tab);
        if (!continue_running) {
            break;
        }

        const bool processed = engine.step();
        write_snapshot_file(engine, options.snapshot_file);

        if (options.live || options.interactive_tabs) {
            render_tui(engine, render_options, !processed);
        }

        continue_running = process_pending_input(input.get(), render_options.active_tab);
        if (!continue_running || !processed) {
            break;
        }

        if (options.sleep_ms > 0) {
            int remaining = options.sleep_ms;
            while (remaining > 0 && continue_running) {
                const int slice = std::min(remaining, 25);
                std::this_thread::sleep_for(std::chrono::milliseconds(slice));
                remaining -= slice;
                continue_running = process_pending_input(input.get(), render_options.active_tab);
                if (continue_running && options.interactive_tabs) {
                    render_tui(engine, render_options, false);
                }
            }
        }
    }

    write_snapshot_file(engine, options.snapshot_file);

    if (options.json) {
        std::cout << engine.dashboard_snapshot_json() << "\n";
        return 0;
    }

    if (options.live || options.interactive_tabs) {
        render_tui(engine, render_options, true);
    } else {
        std::cout << engine.dashboard_terminal(render_options);
    }

    if (options.interactive_tabs) {
        if (input && input->enabled()) {
            while (true) {
                std::this_thread::sleep_for(std::chrono::milliseconds(40));
                const auto command = input->poll_command();
                if (!command.has_value()) {
                    continue;
                }
                if (!apply_tab_command(*command, render_options.active_tab)) {
                    break;
                }
                render_tui(engine, render_options, true);
                write_snapshot_file(engine, options.snapshot_file);
            }
        } else {
            (void)run_blocking_tab_prompt(engine, render_options, options.snapshot_file);
        }
    }

    if (options.ansi_colors) {
        std::cout << "\033[0m\n";
    } else {
        std::cout << "\n";
    }
    return 0;
}
