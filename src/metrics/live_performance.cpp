#include "regimeflow/metrics/live_performance.h"

#include "regimeflow/common/json.h"
#include "regimeflow/common/result.h"
#include "regimeflow/common/types.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

#if defined(REGIMEFLOW_USE_LIBPQ)
#include <libpq-fe.h>
#endif

namespace regimeflow::metrics
{
    namespace {

        bool sink_enabled(const std::vector<std::string>& sinks, const std::string& name) {
            return std::find(sinks.begin(), sinks.end(), name) != sinks.end();
        }

        double extract_number(const common::JsonValue::Object& obj, const std::string& key, double fallback) {
            auto it = obj.find(key);
            if (it == obj.end()) {
                return fallback;
            }
            if (const auto* num = it->second.as_number()) {
                return *num;
            }
            return fallback;
        }

    }  // namespace

    LivePerformanceTracker::LivePerformanceTracker(LivePerformanceConfig config)
        : config_(std::move(config)) {
        if (config_.enabled) {
            load_baseline();
        }
    }

    void LivePerformanceTracker::start(double initial_equity) {
        if (!config_.enabled || initial_equity <= 0.0) {
            return;
        }
        if (!started_) {
            starting_equity_ = initial_equity;
            peak_equity_ = initial_equity;
            summary_.last_equity = initial_equity;
            summary_.last_timestamp = Timestamp::now();
            started_ = true;
        }
    }

    void LivePerformanceTracker::update(Timestamp timestamp, double equity, double daily_pnl) {
        if (!config_.enabled || equity <= 0.0) {
            return;
        }
        if (!started_) {
            start(equity);
        }
        peak_equity_ = std::max(peak_equity_, equity);
        double total_return = starting_equity_ > 0.0
            ? (equity - starting_equity_) / starting_equity_
            : 0.0;
        double drawdown = peak_equity_ > 0.0
            ? (peak_equity_ - equity) / peak_equity_
            : 0.0;
        double shortfall = total_return - summary_.baseline_total_return;

        LivePerformanceSnapshot snapshot;
        snapshot.timestamp = timestamp;
        snapshot.equity = equity;
        snapshot.daily_pnl = daily_pnl;
        snapshot.total_return = total_return;
        snapshot.drawdown = drawdown;
        snapshot.shortfall_vs_backtest = shortfall;

        summary_.live_total_return = total_return;
        summary_.max_drawdown_pct = std::max(summary_.max_drawdown_pct, drawdown);
        summary_.max_drawdown = summary_.max_drawdown_pct;
        summary_.last_equity = equity;
        summary_.last_timestamp = timestamp;

        if (sink_enabled(config_.sinks, "file")) {
            write_file_snapshot(snapshot);
            write_file_summary();
        }
        if (sink_enabled(config_.sinks, "postgres")) {
            write_postgres_snapshot(snapshot);
        }
    }

    void LivePerformanceTracker::flush() {
        if (!config_.enabled) {
            return;
        }
        if (sink_enabled(config_.sinks, "file")) {
            write_file_summary();
        }
    }

    void LivePerformanceTracker::ensure_output_dir() const {
        std::filesystem::path out_dir(config_.output_dir);
        if (out_dir.empty()) {
            return;
        }
        std::error_code ec;
        std::filesystem::create_directories(out_dir, ec);
    }

    void LivePerformanceTracker::write_file_snapshot(const LivePerformanceSnapshot& snapshot) {
        ensure_output_dir();
        std::filesystem::path path = std::filesystem::path(config_.output_dir) / "live_drift.csv";
        bool write_header = !std::filesystem::exists(path);
        std::ofstream out(path, std::ios::app);
        if (!out.is_open()) {
            return;
        }
        if (write_header) {
            out << "timestamp,equity,daily_pnl,total_return,drawdown,shortfall_vs_backtest\n";
        }
        out << snapshot.timestamp.microseconds() << ","
            << snapshot.equity << ","
            << snapshot.daily_pnl << ","
            << snapshot.total_return << ","
            << snapshot.drawdown << ","
            << snapshot.shortfall_vs_backtest << "\n";
    }

    void LivePerformanceTracker::write_file_summary() {
        ensure_output_dir();
        std::filesystem::path path = std::filesystem::path(config_.output_dir) / "live_performance.json";
        std::ofstream out(path, std::ios::trunc);
        if (!out.is_open()) {
            return;
        }
        out << "{\n";
        out << "  \"baseline_total_return\": " << summary_.baseline_total_return << ",\n";
        out << "  \"live_total_return\": " << summary_.live_total_return << ",\n";
        out << "  \"max_drawdown\": " << summary_.max_drawdown << ",\n";
        out << "  \"max_drawdown_pct\": " << summary_.max_drawdown_pct << ",\n";
        out << "  \"last_equity\": " << summary_.last_equity << ",\n";
        out << "  \"last_timestamp\": " << summary_.last_timestamp.microseconds() << "\n";
        out << "}\n";
    }

    void LivePerformanceTracker::write_postgres_snapshot(const LivePerformanceSnapshot& snapshot) {
#if defined(REGIMEFLOW_USE_LIBPQ)
        if (config_.postgres_connection_string.empty()) {
            return;
        }
        PGconn* conn = PQconnectdb(config_.postgres_connection_string.c_str());
        if (!conn || PQstatus(conn) != CONNECTION_OK) {
            if (conn) {
                PQfinish(conn);
            }
            return;
        }
        const std::string create_sql =
            "CREATE TABLE IF NOT EXISTS " + config_.postgres_table +
            " (timestamp BIGINT, equity DOUBLE PRECISION, daily_pnl DOUBLE PRECISION, "
            "total_return DOUBLE PRECISION, drawdown DOUBLE PRECISION, shortfall DOUBLE PRECISION)";
        PGresult* res = PQexec(conn, create_sql.c_str());
        if (res) {
            PQclear(res);
        }
        std::ostringstream insert_sql;
        insert_sql << "INSERT INTO " << config_.postgres_table
                   << " (timestamp, equity, daily_pnl, total_return, drawdown, shortfall) VALUES (";
        insert_sql << snapshot.timestamp.microseconds() << ",";
        insert_sql << snapshot.equity << ",";
        insert_sql << snapshot.daily_pnl << ",";
        insert_sql << snapshot.total_return << ",";
        insert_sql << snapshot.drawdown << ",";
        insert_sql << snapshot.shortfall_vs_backtest << ")";
        res = PQexec(conn, insert_sql.str().c_str());
        if (res) {
            PQclear(res);
        }
        PQfinish(conn);
#else
        (void)snapshot;
#endif
    }

    void LivePerformanceTracker::load_baseline() {
        if (config_.baseline_report.empty()) {
            return;
        }
        std::ifstream in(config_.baseline_report);
        if (!in.is_open()) {
            return;
        }
        std::stringstream buffer;
        buffer << in.rdbuf();
        auto parsed = common::parse_json(buffer.str());
        if (parsed.is_err()) {
            return;
        }
        const auto* root = parsed.value().as_object();
        if (!root) {
            return;
        }
        auto perf_it = root->find("performance");
        if (perf_it != root->end()) {
            if (const auto* perf_obj = perf_it->second.as_object()) {
                summary_.baseline_total_return = extract_number(*perf_obj, "total_return", 0.0);
            }
        }
        auto summary_it = root->find("performance_summary");
        if (summary_it != root->end()) {
            if (const auto* summary_obj = summary_it->second.as_object()) {
                (void)extract_number(*summary_obj, "max_drawdown", 0.0);
            }
        }
    }
}  // namespace regimeflow::metrics
