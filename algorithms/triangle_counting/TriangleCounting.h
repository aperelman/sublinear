#pragma once

#include <vector>
#include <utility>
#include <cstdint>
#include <string>
#include <functional>

struct TriangleCountingConfig {
    double   arboricity  = 3.0;
    double   epsilon     = 0.1;   // error tolerance
    long long T_estimate = 0;     // triangle count from SNAP (0 = use fallback)
    uint64_t seed        = 42;
};

struct TriangleCountingStats {
    double total_weight = 0.0;
    size_t r            = 0;      // actual sample size used
};

using TriangleCountingResult = std::pair<double, TriangleCountingStats>;

class TriangleCounting {
public:
    static TriangleCountingResult count(
        const std::vector<std::pair<int, int>>& edges,
        const TriangleCountingConfig& config);
};
