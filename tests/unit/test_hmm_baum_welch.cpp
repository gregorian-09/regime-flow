#include <gtest/gtest.h>

#include "regimeflow/regime/hmm.h"

#include <cmath>
#include <random>

namespace regimeflow::test {

static std::vector<regimeflow::regime::FeatureVector> generate_sequence(
    int length,
    const std::vector<std::vector<double>>& transition,
    const std::vector<double>& means,
    const std::vector<double>& vars) {
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> uni(0.0, 1.0);
    std::vector<std::normal_distribution<double>> normals;
    for (size_t i = 0; i < means.size(); ++i) {
        normals.emplace_back(means[i], std::sqrt(vars[i]));
    }
    std::vector<regimeflow::regime::FeatureVector> data;
    data.reserve(length);
    int state = 0;
    for (int t = 0; t < length; ++t) {
        double r = uni(rng);
        double cumulative = 0.0;
        for (size_t j = 0; j < transition[state].size(); ++j) {
            cumulative += transition[state][j];
            if (r <= cumulative) {
                state = static_cast<int>(j);
                break;
            }
        }
        data.push_back({normals[state](rng)});
    }
    return data;
}

TEST(HMMBaumWelch, ImprovesLogLikelihood) {
    std::vector<std::vector<double>> transition = {{0.9, 0.1}, {0.1, 0.9}};
    std::vector<double> means = {-1.0, 1.0};
    std::vector<double> vars = {0.2, 0.2};

    auto data = generate_sequence(200, transition, means, vars);

    regimeflow::regime::HMMRegimeDetector hmm(2, 10);
    double before = hmm.log_likelihood(data);
    hmm.baum_welch(data, 25, 1e-3);
    double after = hmm.log_likelihood(data);

    EXPECT_GT(after, before);
}

TEST(HMMBaumWelch, LogLikelihoodFiniteForLongSequence) {
    std::vector<std::vector<double>> transition = {{0.95, 0.05}, {0.05, 0.95}};
    std::vector<double> means = {-0.5, 0.5};
    std::vector<double> vars = {0.05, 0.05};
    auto data = generate_sequence(1000, transition, means, vars);

    regimeflow::regime::HMMRegimeDetector hmm(2, 10);
    double ll = hmm.log_likelihood(data);
    EXPECT_TRUE(std::isfinite(ll));
}

}  // namespace regimeflow::test
