/**
 * @file kalman_filter.h
 * @brief RegimeFlow regimeflow kalman filter declarations.
 */

#pragma once

#include <algorithm>

namespace regimeflow::regime {

/**
 * @brief Simple 1D Kalman filter for smoothing signals.
 */
class KalmanFilter1D {
public:
    KalmanFilter1D() = default;
    /**
     * @brief Construct with noise parameters.
     * @param process_noise Process noise variance.
     * @param measurement_noise Measurement noise variance.
     */
    KalmanFilter1D(double process_noise, double measurement_noise)
        : q_(process_noise), r_(measurement_noise) {}

    /**
     * @brief Configure noise parameters.
     * @param process_noise Process noise variance.
     * @param measurement_noise Measurement noise variance.
     */
    void configure(double process_noise, double measurement_noise) {
        q_ = process_noise;
        r_ = measurement_noise;
    }

    /**
     * @brief Reset filter state.
     */
    void reset() {
        initialized_ = false;
        x_ = 0.0;
        p_ = 1.0;
    }

    /**
     * @brief Update the filter with a measurement.
     * @param measurement Measurement value.
     * @return Filtered estimate.
     */
    double update(double measurement) {
        if (!initialized_) {
            x_ = measurement;
            p_ = 1.0;
            initialized_ = true;
            return x_;
        }
        p_ += q_;
        double k = p_ / (p_ + r_);
        x_ = x_ + k * (measurement - x_);
        p_ = (1.0 - k) * p_;
        return x_;
    }

private:
    double x_ = 0.0;
    double p_ = 1.0;
    double q_ = 1e-3;
    double r_ = 1e-2;
    bool initialized_ = false;
};

}  // namespace regimeflow::regime
