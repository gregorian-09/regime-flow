/**
 * @file regime_detector.h
 * @brief RegimeFlow regimeflow regime detector declarations.
 */

#pragma once

#include "regimeflow/common/config.h"
#include "regimeflow/data/bar.h"
#include "regimeflow/data/order_book.h"
#include "regimeflow/data/tick.h"
#include "regimeflow/regime/types.h"
#include "regimeflow/regime/features.h"

#include <cstdint>
#include <string>
#include <vector>

namespace regimeflow::regime
{
    /**
     * @brief User-facing governance metadata for persisted regime models.
     */
    struct ModelGovernanceMetadata {
        std::string detector_type;
        std::string model_version = "unversioned";
        int64_t training_start_us = 0;
        int64_t training_end_us = 0;
        std::string feature_schema;
        std::string parameter_digest;
    };

    /**
     * @brief Abstract interface for regime detectors.
     */
    class RegimeDetector {
    public:
        virtual ~RegimeDetector() = default;

        /**
         * @brief Update detector with bar data.
         * @param bar Bar data.
         * @return Current regime state.
         */
        virtual RegimeState on_bar(const data::Bar& bar) = 0;
        /**
         * @brief Update detector with tick data.
         * @param tick Tick data.
         * @return Current regime state.
         */
        virtual RegimeState on_tick(const data::Tick& tick) = 0;
        /**
         * @brief Update detector with order book data.
         * @param book Order book snapshot.
         * @return Current regime state.
         */
        virtual RegimeState on_book(const data::OrderBook& book) {
            data::Bar bar{};
            bar.timestamp = book.timestamp;
            bar.symbol = book.symbol;
            double bid = book.bids[0].price;
            double ask = book.asks[0].price;
            double mid = (bid + ask) / 2.0;
            bar.open = mid;
            bar.high = mid;
            bar.low = mid;
            bar.close = mid;
            bar.volume = 0;
            return on_bar(bar);
        }
        /**
         * @brief Train the detector with feature vectors.
         * @param features Feature vectors.
         */
        virtual void train(const std::vector<FeatureVector>&) {}
        /**
         * @brief Save detector state to disk.
         * @param path Output path.
         */
        virtual void save(const std::string& path) const { (void)path; }
        /**
         * @brief Load detector state from disk.
         * @param path Input path.
         */
        virtual void load(const std::string& path) { (void)path; }
        /**
         * @brief Configure detector with parameters.
         * @param config Configuration.
         */
        virtual void configure(const Config& config) { (void)config; }
        /**
         * @brief Number of states in the model (if applicable).
         */
        [[nodiscard]] virtual int num_states() const { return 0; }
        /**
         * @brief State names for display.
         */
        [[nodiscard]] virtual std::vector<std::string> state_names() const { return {}; }
        /**
         * @brief Return model governance metadata for audit/replay records.
         */
        [[nodiscard]] virtual ModelGovernanceMetadata model_metadata() const { return {}; }
        /**
         * @brief Set model governance metadata.
         */
        virtual void set_model_metadata(ModelGovernanceMetadata metadata) { (void)metadata; }
    };
}  // namespace regimeflow::regime
