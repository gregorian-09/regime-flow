#pragma once

#include "regimeflow/common/result.h"
#include "regimeflow/data/data_source.h"
#include "regimeflow/data/data_validation.h"
#include "regimeflow/data/validation_config.h"

#include <functional>
#include <string>
#include <vector>

namespace regimeflow::data {

class ApiDataSource : public DataSource {
public:
    struct Config {
        std::string base_url;
        std::string bars_endpoint = "/bars";
        std::string ticks_endpoint = "/ticks";
        std::string api_key;
        std::string api_key_header = "X-API-KEY";
        std::string format = "csv";
        std::string time_format = "epoch_ms";
        int timeout_seconds = 10;
        std::vector<std::string> symbols;
        ValidationConfig validation;
        bool collect_validation_report = false;
        bool fill_missing_bars = false;
    };

    explicit ApiDataSource(Config config);

    std::vector<SymbolInfo> get_available_symbols() const override;
    TimeRange get_available_range(SymbolId symbol) const override;

    std::vector<Bar> get_bars(SymbolId symbol, TimeRange range,
                              BarType bar_type = BarType::Time_1Day) override;
    std::vector<Tick> get_ticks(SymbolId symbol, TimeRange range) override;

    std::unique_ptr<DataIterator> create_iterator(
        const std::vector<SymbolId>& symbols,
        TimeRange range,
        BarType bar_type) override;

    std::vector<CorporateAction> get_corporate_actions(SymbolId symbol,
                                                       TimeRange range) override;
    const ValidationReport& last_report() const { return last_report_; }

private:
    Config config_;
    mutable ValidationReport last_report_;

    std::string build_url(const std::string& endpoint,
                          const std::string& symbol,
                          TimeRange range,
                          BarType bar_type) const;
    std::string format_timestamp(Timestamp ts) const;
};

}  // namespace regimeflow::data
