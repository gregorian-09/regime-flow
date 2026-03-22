#include "regimeflow/engine/order_routing.h"

#include "regimeflow/common/config.h"

#include <algorithm>
#include <cstddef>
#include <cctype>
#include <cmath>
#include <numeric>
#include <string_view>

namespace regimeflow::engine
{
    namespace {
        std::string to_lower(std::string value) {
            std::transform(value.begin(), value.end(), value.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return value;
        }

        std::optional<std::string> get_string(const Config& cfg, const std::string& key) {
            return cfg.get_as<std::string>(key);
        }

        std::optional<double> get_double(const Config& cfg, const std::string& key) {
            return cfg.get_as<double>(key);
        }

        std::optional<int64_t> get_int(const Config& cfg, const std::string& key) {
            return cfg.get_as<int64_t>(key);
        }

        std::optional<bool> get_bool(const Config& cfg, const std::string& key) {
            return cfg.get_as<bool>(key);
        }

        RoutingMode parse_routing_mode(const std::string& value) {
            const auto lower = to_lower(value);
            if (lower == "none" || lower == "off" || lower == "disabled") {
                return RoutingMode::None;
            }
            return RoutingMode::Smart;
        }

        SplitMode parse_split_mode(const std::string& value) {
            const auto lower = to_lower(value);
            if (lower == "parallel") {
                return SplitMode::Parallel;
            }
            if (lower == "sequential") {
                return SplitMode::Sequential;
            }
            return SplitMode::None;
        }

        ParentAggregation parse_parent_aggregation(const std::string& value) {
            const auto lower = to_lower(value);
            if (lower == "none") {
                return ParentAggregation::None;
            }
            if (lower == "final") {
                return ParentAggregation::Final;
            }
            return ParentAggregation::Partial;
        }

        std::optional<RoutingVenue> parse_venue(const ConfigValue& value) {
            if (const auto* obj = value.get_if<ConfigValue::Object>()) {
                RoutingVenue venue;
                auto it = obj->find("name");
                if (it != obj->end()) {
                    if (const auto* name = it->second.get_if<std::string>()) {
                        venue.name = *name;
                    }
                }
                it = obj->find("weight");
                if (it != obj->end()) {
                    if (const auto* w = it->second.get_if<double>()) {
                        venue.weight = *w;
                    } else if (const auto* wi = it->second.get_if<int64_t>()) {
                        venue.weight = static_cast<double>(*wi);
                    }
                }
                it = obj->find("maker_rebate_bps");
                if (it != obj->end()) {
                    if (const auto* v = it->second.get_if<double>()) {
                        venue.maker_rebate_bps = *v;
                    } else if (const auto* vi = it->second.get_if<int64_t>()) {
                        venue.maker_rebate_bps = static_cast<double>(*vi);
                    }
                }
                it = obj->find("taker_fee_bps");
                if (it != obj->end()) {
                    if (const auto* v = it->second.get_if<double>()) {
                        venue.taker_fee_bps = *v;
                    } else if (const auto* vi = it->second.get_if<int64_t>()) {
                        venue.taker_fee_bps = static_cast<double>(*vi);
                    }
                }
                it = obj->find("price_adjustment_bps");
                if (it != obj->end()) {
                    if (const auto* v = it->second.get_if<double>()) {
                        venue.price_adjustment_bps = *v;
                    } else if (const auto* vi = it->second.get_if<int64_t>()) {
                        venue.price_adjustment_bps = static_cast<double>(*vi);
                    }
                }
                it = obj->find("latency_ms");
                if (it != obj->end()) {
                    if (const auto* v = it->second.get_if<int64_t>()) {
                        venue.latency_ms = *v;
                    } else if (const auto* vd = it->second.get_if<double>()) {
                        venue.latency_ms = static_cast<int64_t>(*vd);
                    }
                }
                it = obj->find("queue_enabled");
                if (it != obj->end()) {
                    if (const auto* v = it->second.get_if<bool>()) {
                        venue.queue_enabled = *v;
                    }
                }
                it = obj->find("queue_progress_fraction");
                if (it != obj->end()) {
                    if (const auto* v = it->second.get_if<double>()) {
                        venue.queue_progress_fraction = *v;
                    } else if (const auto* vi = it->second.get_if<int64_t>()) {
                        venue.queue_progress_fraction = static_cast<double>(*vi);
                    }
                }
                it = obj->find("queue_default_visible_qty");
                if (it != obj->end()) {
                    if (const auto* v = it->second.get_if<double>()) {
                        venue.queue_default_visible_qty = *v;
                    } else if (const auto* vi = it->second.get_if<int64_t>()) {
                        venue.queue_default_visible_qty = static_cast<double>(*vi);
                    }
                }
                it = obj->find("queue_depth_mode");
                if (it != obj->end()) {
                    if (const auto* v = it->second.get_if<std::string>()) {
                        venue.queue_depth_mode = *v;
                    }
                }
                if (!venue.name.empty()) {
                    return venue;
                }
            }
            if (const auto* name = value.get_if<std::string>()) {
                RoutingVenue venue;
                venue.name = *name;
                return venue;
            }
            return std::nullopt;
        }

        Price apply_offset(const Price price, const double offset_bps, const bool add) {
            if (price <= 0.0 || offset_bps == 0.0) {
                return price;
            }
            const double factor = offset_bps / 10000.0;
            return add ? price * (1.0 + factor) : price * (1.0 - factor);
        }

        void clear_venue_profile(Order& order) {
            order.metadata.erase("venue_maker_rebate_bps");
            order.metadata.erase("venue_taker_fee_bps");
            order.metadata.erase("venue_price_adjustment_bps");
            order.metadata.erase("venue_latency_ms");
            order.metadata.erase("venue_queue_enabled");
            order.metadata.erase("venue_queue_progress_fraction");
            order.metadata.erase("venue_queue_default_visible_qty");
            order.metadata.erase("venue_queue_depth_mode");
        }

        void apply_venue_profile(Order& order, const RoutingVenue* venue) {
            clear_venue_profile(order);
            if (venue == nullptr) {
                return;
            }
            if (venue->maker_rebate_bps.has_value()) {
                order.metadata["venue_maker_rebate_bps"] = std::to_string(*venue->maker_rebate_bps);
            }
            if (venue->taker_fee_bps.has_value()) {
                order.metadata["venue_taker_fee_bps"] = std::to_string(*venue->taker_fee_bps);
            }
            if (venue->price_adjustment_bps.has_value()) {
                order.metadata["venue_price_adjustment_bps"] = std::to_string(*venue->price_adjustment_bps);
            }
            if (venue->latency_ms.has_value()) {
                order.metadata["venue_latency_ms"] = std::to_string(*venue->latency_ms);
            }
            if (venue->queue_enabled.has_value()) {
                order.metadata["venue_queue_enabled"] = *venue->queue_enabled ? "true" : "false";
            }
            if (venue->queue_progress_fraction.has_value()) {
                order.metadata["venue_queue_progress_fraction"] = std::to_string(*venue->queue_progress_fraction);
            }
            if (venue->queue_default_visible_qty.has_value()) {
                order.metadata["venue_queue_default_visible_qty"] = std::to_string(*venue->queue_default_visible_qty);
            }
            if (venue->queue_depth_mode.has_value()) {
                order.metadata["venue_queue_depth_mode"] = *venue->queue_depth_mode;
            }
        }
    }  // namespace

    RoutingConfig RoutingConfig::from_config(const Config& config, const std::string& prefix) {
        RoutingConfig cfg;
        const auto enabled = get_bool(config, prefix + ".enabled");
        cfg.enabled = enabled.value_or(false);
        if (const auto mode = get_string(config, prefix + ".mode")) {
            cfg.mode = parse_routing_mode(*mode);
        }
        if (const auto venue = get_string(config, prefix + ".default_venue")) {
            cfg.default_venue = *venue;
        }
        if (const auto bps = get_double(config, prefix + ".max_spread_bps")) {
            cfg.max_spread_bps = *bps;
        }
        if (const auto bps = get_double(config, prefix + ".passive_offset_bps")) {
            cfg.passive_offset_bps = *bps;
        }
        if (const auto bps = get_double(config, prefix + ".aggressive_offset_bps")) {
            cfg.aggressive_offset_bps = *bps;
        }
        if (const auto qty = get_double(config, prefix + ".split.min_child_qty")) {
            cfg.min_child_qty = *qty;
        }
        if (const auto max = get_int(config, prefix + ".split.max_children")) {
            cfg.max_children = static_cast<int>(*max);
        }
        if (const auto split_enabled = get_bool(config, prefix + ".split.enabled")) {
            if (*split_enabled) {
                cfg.split_mode = SplitMode::Parallel;
            }
        }
        if (const auto split_mode = get_string(config, prefix + ".split.mode")) {
            cfg.split_mode = parse_split_mode(*split_mode);
        }
        if (const auto agg = get_string(config, prefix + ".split.parent_aggregation")) {
            cfg.parent_aggregation = parse_parent_aggregation(*agg);
        }
        if (cfg.split_enabled()) {
            cfg.enabled = true;
        }
        if (const auto venues_val = config.get_as<ConfigValue::Array>(prefix + ".venues")) {
            for (const auto& entry : *venues_val) {
                if (auto venue = parse_venue(entry)) {
                    cfg.venues.push_back(std::move(*venue));
                }
            }
        }
        return cfg;
    }

    std::optional<Price> RoutingContext::mid() const {
        if (bid && ask) {
            return (*bid + *ask) / 2.0;
        }
        return std::nullopt;
    }

    std::optional<double> RoutingContext::spread_bps() const {
        if (!bid || !ask) {
            return std::nullopt;
        }
        const double mid_price = (*bid + *ask) / 2.0;
        if (mid_price <= 0.0) {
            return std::nullopt;
        }
        return (*ask - *bid) / mid_price * 10000.0;
    }

    SmartOrderRouter::SmartOrderRouter(RoutingConfig config) : config_(std::move(config)) {}

    RoutingPlan SmartOrderRouter::route(const Order& order, const RoutingContext& ctx) const {
        RoutingPlan plan;
        plan.routed_order = order;
        plan.split_mode = config_.split_mode;
        plan.parent_aggregation = config_.parent_aggregation;

        if (!config_.enabled || config_.mode == RoutingMode::None) {
            return plan;
        }

        const auto find_venue = [this](const std::string& name) -> const RoutingVenue* {
            for (const auto& venue : config_.venues) {
                if (venue.name == name) {
                    return &venue;
                }
            }
            return nullptr;
        };

        if (plan.routed_order.metadata.contains("venue")) {
            apply_venue_profile(plan.routed_order, find_venue(plan.routed_order.metadata.at("venue")));
        } else if (!config_.venues.empty()) {
            const auto best = std::max_element(
                config_.venues.begin(),
                config_.venues.end(),
                [](const RoutingVenue& a, const RoutingVenue& b) {
                    return a.weight < b.weight;
                });
            if (best != config_.venues.end()) {
                plan.routed_order.metadata["venue"] = best->name;
                apply_venue_profile(plan.routed_order, &(*best));
            }
        } else if (!config_.default_venue.empty()) {
            plan.routed_order.metadata["venue"] = config_.default_venue;
            apply_venue_profile(plan.routed_order, find_venue(config_.default_venue));
        }

        std::string route_style = "auto";
        if (const auto it = plan.routed_order.metadata.find("route_style");
            it != plan.routed_order.metadata.end()) {
            route_style = to_lower(it->second);
        } else if (const auto it = plan.routed_order.metadata.find("router");
                   it != plan.routed_order.metadata.end()) {
            route_style = to_lower(it->second);
        }

        const bool has_book = ctx.bid.has_value() && ctx.ask.has_value();
        const bool has_last = ctx.last.has_value();
        const auto spread = ctx.spread_bps();
        const bool spread_ok = spread.has_value() && *spread <= config_.max_spread_bps;

        auto choose_price = [&](const bool aggressive) -> std::optional<Price> {
            if (has_book) {
                if (aggressive) {
                    if (plan.routed_order.side == OrderSide::Buy) {
                        return apply_offset(*ctx.ask, config_.aggressive_offset_bps, true);
                    }
                    return apply_offset(*ctx.bid, config_.aggressive_offset_bps, false);
                }
                if (plan.routed_order.side == OrderSide::Buy) {
                    return apply_offset(*ctx.bid, config_.passive_offset_bps, false);
                }
                return apply_offset(*ctx.ask, config_.passive_offset_bps, true);
            }
            if (has_last) {
                return *ctx.last;
            }
            return std::nullopt;
        };

        bool force_limit = false;
        bool aggressive = false;
        if (route_style == "passive" || route_style == "limit") {
            force_limit = true;
        } else if (route_style == "aggressive" || route_style == "market") {
            force_limit = true;
            aggressive = true;
        } else if (plan.routed_order.type == OrderType::Market && spread_ok) {
            force_limit = true;
        }

        if (force_limit) {
            if (const auto price = choose_price(aggressive)) {
                plan.routed_order.type = OrderType::Limit;
                plan.routed_order.limit_price = *price;
                plan.routed_order.metadata["router"] = "limit";
                plan.routed_order.metadata["route_style"] = aggressive ? "aggressive" : "passive";
            }
        } else if (aggressive && plan.routed_order.type == OrderType::Limit &&
                   plan.routed_order.limit_price <= 0.0) {
            if (const auto price = choose_price(true)) {
                plan.routed_order.limit_price = *price;
            }
        }

        if (config_.split_enabled() && !config_.venues.empty()) {
            const size_t max_children = static_cast<size_t>(
                std::max(1, std::min(config_.max_children,
                                     static_cast<int>(config_.venues.size()))));
            if (max_children >= 2) {
                const double required_qty = config_.min_child_qty * static_cast<double>(max_children);
                if (plan.routed_order.quantity >= required_qty) {
                    const auto total_weight = std::accumulate(
                        config_.venues.begin(),
                        config_.venues.begin() + static_cast<std::ptrdiff_t>(max_children),
                        0.0,
                        [](const double sum, const RoutingVenue& venue) {
                            return sum + std::max(0.0, venue.weight);
                        });
                    if (total_weight > 0.0) {
                        double remaining = plan.routed_order.quantity;
                        for (size_t i = 0; i < max_children; ++i) {
                            const auto& venue = config_.venues[i];
                            double weight = std::max(0.0, venue.weight);
                            double qty = (i + 1 == max_children)
                                ? remaining
                                : plan.routed_order.quantity * (weight / total_weight);
                            if (qty < config_.min_child_qty) {
                                qty = config_.min_child_qty;
                            }
                            if (qty > remaining) {
                                qty = remaining;
                            }
                            Order child = plan.routed_order;
                            child.quantity = qty;
                            child.metadata["venue"] = venue.name;
                            child.metadata["route_parent"] = "split";
                            apply_venue_profile(child, &venue);
                            plan.children.push_back(std::move(child));
                            remaining -= qty;
                            if (remaining <= 0.0) {
                                break;
                            }
                        }
                        if (!plan.children.empty()) {
                            plan.routed_order.metadata["route_mode"] = "split";
                        }
                    }
                }
            }
        }

        return plan;
    }
}  // namespace regimeflow::engine
