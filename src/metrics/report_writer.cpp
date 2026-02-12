#include "regimeflow/metrics/report_writer.h"

#include <sstream>

namespace regimeflow::metrics {

namespace {

const char* regime_name(regime::RegimeType regime) {
    switch (regime) {
        case regime::RegimeType::Bull: return "bull";
        case regime::RegimeType::Neutral: return "neutral";
        case regime::RegimeType::Bear: return "bear";
        case regime::RegimeType::Crisis: return "crisis";
        default: return "custom";
    }
}

std::string format_date(const Timestamp& ts, const char* fmt) {
    if (ts.microseconds() == 0) {
        return "";
    }
    return ts.to_string(fmt);
}

}  // namespace

std::string ReportWriter::to_csv(const Report& report) {
    std::ostringstream out;
    out << "metric,value\n";
    out << "total_return," << report.performance.total_return << "\n";
    out << "cagr," << report.performance.cagr << "\n";
    out << "volatility," << report.performance.volatility << "\n";
    out << "sharpe," << report.performance.sharpe << "\n";
    out << "sortino," << report.performance.sortino << "\n";
    out << "calmar," << report.performance.calmar << "\n";
    out << "var_95," << report.performance.var_95 << "\n";
    out << "cvar_95," << report.performance.cvar_95 << "\n";
    out << "best_return," << report.performance.best_return << "\n";
    out << "worst_return," << report.performance.worst_return << "\n";
    out << "summary_total_return," << report.performance_summary.total_return << "\n";
    out << "summary_cagr," << report.performance_summary.cagr << "\n";
    out << "summary_avg_daily_return," << report.performance_summary.avg_daily_return << "\n";
    out << "summary_avg_monthly_return," << report.performance_summary.avg_monthly_return << "\n";
    out << "summary_best_day," << report.performance_summary.best_day << "\n";
    out << "summary_worst_day," << report.performance_summary.worst_day << "\n";
    out << "summary_best_day_date," << format_date(report.performance_summary.best_day_date, "%Y-%m-%d")
        << "\n";
    out << "summary_worst_day_date," << format_date(report.performance_summary.worst_day_date, "%Y-%m-%d")
        << "\n";
    out << "summary_best_month," << report.performance_summary.best_month << "\n";
    out << "summary_worst_month," << report.performance_summary.worst_month << "\n";
    out << "summary_best_month_date," << format_date(report.performance_summary.best_month_date, "%Y-%m-%d")
        << "\n";
    out << "summary_worst_month_date," << format_date(report.performance_summary.worst_month_date, "%Y-%m-%d")
        << "\n";
    out << "summary_volatility," << report.performance_summary.volatility << "\n";
    out << "summary_downside_deviation," << report.performance_summary.downside_deviation << "\n";
    out << "summary_max_drawdown," << report.performance_summary.max_drawdown << "\n";
    out << "summary_var_95," << report.performance_summary.var_95 << "\n";
    out << "summary_var_99," << report.performance_summary.var_99 << "\n";
    out << "summary_cvar_95," << report.performance_summary.cvar_95 << "\n";
    out << "summary_sharpe_ratio," << report.performance_summary.sharpe_ratio << "\n";
    out << "summary_sortino_ratio," << report.performance_summary.sortino_ratio << "\n";
    out << "summary_calmar_ratio," << report.performance_summary.calmar_ratio << "\n";
    out << "summary_omega_ratio," << report.performance_summary.omega_ratio << "\n";
    out << "summary_ulcer_index," << report.performance_summary.ulcer_index << "\n";
    out << "summary_information_ratio," << report.performance_summary.information_ratio << "\n";
    out << "summary_treynor_ratio," << report.performance_summary.treynor_ratio << "\n";
    out << "summary_tail_ratio," << report.performance_summary.tail_ratio << "\n";
    out << "summary_total_trades," << report.performance_summary.total_trades << "\n";
    out << "summary_winning_trades," << report.performance_summary.winning_trades << "\n";
    out << "summary_losing_trades," << report.performance_summary.losing_trades << "\n";
    out << "summary_open_trades," << report.performance_summary.open_trades << "\n";
    out << "summary_closed_trades," << report.performance_summary.closed_trades << "\n";
    out << "summary_open_trades_unrealized_pnl,"
        << report.performance_summary.open_trades_unrealized_pnl << "\n";
    out << "summary_open_trades_snapshot_date,"
        << format_date(report.performance_summary.open_trades_snapshot_date, "%Y-%m-%d")
        << "\n";
    out << "summary_win_rate," << report.performance_summary.win_rate << "\n";
    out << "summary_profit_factor," << report.performance_summary.profit_factor << "\n";
    out << "summary_avg_win," << report.performance_summary.avg_win << "\n";
    out << "summary_avg_loss," << report.performance_summary.avg_loss << "\n";
    out << "summary_win_loss_ratio," << report.performance_summary.win_loss_ratio << "\n";
    out << "summary_expectancy," << report.performance_summary.expectancy << "\n";
    out << "summary_avg_trade_duration_days," << report.performance_summary.avg_trade_duration_days
        << "\n";
    out << "summary_annual_turnover," << report.performance_summary.annual_turnover << "\n";
    out << "max_drawdown," << report.max_drawdown << "\n";
    out << "regime,return,avg_return,sharpe,max_drawdown,time_pct,observations\n";
    for (const auto& [regime, perf] : report.regime_performance) {
        out << regime_name(regime) << "," << perf.total_return << "," << perf.avg_return << ","
            << perf.sharpe << "," << perf.max_drawdown << "," << perf.time_pct << ","
            << perf.observations << "\n";
    }
    out << "transition_from,transition_to,avg_return,volatility,observations\n";
    for (const auto& [key, stats] : report.transitions) {
        out << regime_name(key.first) << "," << regime_name(key.second) << ","
            << stats.avg_return << "," << stats.volatility << "," << stats.observations << "\n";
    }
    return out.str();
}

std::string ReportWriter::to_json(const Report& report) {
    std::ostringstream out;
    out << "{\n";
    out << "  \"performance\": {\n";
    out << "    \"total_return\": " << report.performance.total_return << ",\n";
    out << "    \"cagr\": " << report.performance.cagr << ",\n";
    out << "    \"volatility\": " << report.performance.volatility << ",\n";
    out << "    \"sharpe\": " << report.performance.sharpe << ",\n";
    out << "    \"sortino\": " << report.performance.sortino << ",\n";
    out << "    \"calmar\": " << report.performance.calmar << ",\n";
    out << "    \"var_95\": " << report.performance.var_95 << ",\n";
    out << "    \"cvar_95\": " << report.performance.cvar_95 << ",\n";
    out << "    \"best_return\": " << report.performance.best_return << ",\n";
    out << "    \"worst_return\": " << report.performance.worst_return << "\n";
    out << "  },\n";
    out << "  \"performance_summary\": {\n";
    out << "    \"total_return\": " << report.performance_summary.total_return << ",\n";
    out << "    \"cagr\": " << report.performance_summary.cagr << ",\n";
    out << "    \"avg_daily_return\": " << report.performance_summary.avg_daily_return << ",\n";
    out << "    \"avg_monthly_return\": " << report.performance_summary.avg_monthly_return << ",\n";
    out << "    \"best_day\": " << report.performance_summary.best_day << ",\n";
    out << "    \"worst_day\": " << report.performance_summary.worst_day << ",\n";
    out << "    \"best_day_date\": \""
        << format_date(report.performance_summary.best_day_date, "%Y-%m-%d") << "\",\n";
    out << "    \"worst_day_date\": \""
        << format_date(report.performance_summary.worst_day_date, "%Y-%m-%d") << "\",\n";
    out << "    \"best_month\": " << report.performance_summary.best_month << ",\n";
    out << "    \"worst_month\": " << report.performance_summary.worst_month << ",\n";
    out << "    \"best_month_date\": \""
        << format_date(report.performance_summary.best_month_date, "%Y-%m-%d") << "\",\n";
    out << "    \"worst_month_date\": \""
        << format_date(report.performance_summary.worst_month_date, "%Y-%m-%d") << "\",\n";
    out << "    \"volatility\": " << report.performance_summary.volatility << ",\n";
    out << "    \"downside_deviation\": " << report.performance_summary.downside_deviation << ",\n";
    out << "    \"max_drawdown\": " << report.performance_summary.max_drawdown << ",\n";
    out << "    \"var_95\": " << report.performance_summary.var_95 << ",\n";
    out << "    \"var_99\": " << report.performance_summary.var_99 << ",\n";
    out << "    \"cvar_95\": " << report.performance_summary.cvar_95 << ",\n";
    out << "    \"sharpe_ratio\": " << report.performance_summary.sharpe_ratio << ",\n";
    out << "    \"sortino_ratio\": " << report.performance_summary.sortino_ratio << ",\n";
    out << "    \"calmar_ratio\": " << report.performance_summary.calmar_ratio << ",\n";
    out << "    \"omega_ratio\": " << report.performance_summary.omega_ratio << ",\n";
    out << "    \"ulcer_index\": " << report.performance_summary.ulcer_index << ",\n";
    out << "    \"information_ratio\": " << report.performance_summary.information_ratio << ",\n";
    out << "    \"treynor_ratio\": " << report.performance_summary.treynor_ratio << ",\n";
    out << "    \"tail_ratio\": " << report.performance_summary.tail_ratio << ",\n";
    out << "    \"total_trades\": " << report.performance_summary.total_trades << ",\n";
    out << "    \"winning_trades\": " << report.performance_summary.winning_trades << ",\n";
    out << "    \"losing_trades\": " << report.performance_summary.losing_trades << ",\n";
    out << "    \"open_trades\": " << report.performance_summary.open_trades << ",\n";
    out << "    \"closed_trades\": " << report.performance_summary.closed_trades << ",\n";
    out << "    \"open_trades_unrealized_pnl\": "
        << report.performance_summary.open_trades_unrealized_pnl << ",\n";
    out << "    \"open_trades_snapshot_date\": \""
        << format_date(report.performance_summary.open_trades_snapshot_date, "%Y-%m-%d") << "\",\n";
    out << "    \"win_rate\": " << report.performance_summary.win_rate << ",\n";
    out << "    \"profit_factor\": " << report.performance_summary.profit_factor << ",\n";
    out << "    \"avg_win\": " << report.performance_summary.avg_win << ",\n";
    out << "    \"avg_loss\": " << report.performance_summary.avg_loss << ",\n";
    out << "    \"win_loss_ratio\": " << report.performance_summary.win_loss_ratio << ",\n";
    out << "    \"expectancy\": " << report.performance_summary.expectancy << ",\n";
    out << "    \"avg_trade_duration_days\": "
        << report.performance_summary.avg_trade_duration_days << ",\n";
    out << "    \"annual_turnover\": " << report.performance_summary.annual_turnover << "\n";
    out << "  },\n";
    out << "  \"max_drawdown\": " << report.max_drawdown << ",\n";
    out << "  \"regime_performance\": [\n";
    bool first = true;
    for (const auto& [regime, perf] : report.regime_performance) {
        if (!first) {
            out << ",\n";
        }
        first = false;
        out << "    {\"regime\": \"" << regime_name(regime) << "\", \"return\": "
            << perf.total_return << ", \"avg_return\": " << perf.avg_return
            << ", \"sharpe\": " << perf.sharpe
            << ", \"max_drawdown\": " << perf.max_drawdown
            << ", \"time_pct\": " << perf.time_pct
            << ", \"observations\": " << perf.observations << "}";
    }
    out << "\n  ],\n";
    out << "  \"transitions\": [\n";
    first = true;
    for (const auto& [key, stats] : report.transitions) {
        if (!first) {
            out << ",\n";
        }
        first = false;
        out << "    {\"from\": \"" << regime_name(key.first) << "\", \"to\": \""
            << regime_name(key.second) << "\", \"avg_return\": " << stats.avg_return
            << ", \"volatility\": " << stats.volatility
            << ", \"observations\": " << stats.observations << "}";
    }
    out << "\n  ]\n";
    out << "}\n";
    return out.str();
}

}  // namespace regimeflow::metrics
