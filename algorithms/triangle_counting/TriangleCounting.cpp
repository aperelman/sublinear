#include "TriangleCounting.h"
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cmath>

using AdjMap = std::unordered_map<int, std::unordered_set<int>>;

static AdjMap buildAdjacency(const std::vector<std::pair<int,int>>& edges) {
    AdjMap adj;
    for (auto& [u, v] : edges)
        if (u != v) { adj[u].insert(v); adj[v].insert(u); }
    return adj;
}

static std::vector<std::pair<int,int>>
deduplicateEdges(const std::vector<std::pair<int,int>>& edges) {
    std::vector<std::pair<int,int>> result;
    result.reserve(edges.size());
    for (auto& [u, v] : edges)
        if (u != v)
            result.emplace_back(std::min(u,v), std::max(u,v));
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

TriangleCountingResult TriangleCounting::count(
    const std::vector<std::pair<int,int>>& edges,
    const TriangleCountingConfig& config)
{
    if (edges.empty()) return {0.0, {}};

    auto adj      = buildAdjacency(edges);
    auto allEdges = deduplicateEdges(edges);
    size_t m      = allEdges.size();
    if (m == 0) return {0.0, {}};

    std::mt19937_64 rng(config.seed);
    std::uniform_real_distribution<double> uniform01(0.0, 1.0);

    // -----------------------------------------------------------
    // Compute r per paper:  r = m * alpha / (epsilon * t')
    // where t' = 0.1 * T  (T from SNAP)
    // Fallback: r = m (sample all edges) if T not known
    // -----------------------------------------------------------
    size_t r;
    if (config.T_estimate > 0) {
        double t_prime = 0.1 * (double)config.T_estimate;
        double r_exact = ((double)m * config.arboricity) / (config.epsilon * t_prime);
        r = std::min((size_t)std::ceil(r_exact), m);
    } else {
        r = m; // fallback: use all edges
    }

    double p1 = (double)r / (double)m;

    // -----------------------------------------------------------
    // Stage 1: Uniform edge sparsification — sample R ⊆ E
    // -----------------------------------------------------------
    std::vector<std::pair<int,int>> R;
    R.reserve(r);
    for (auto& e : allEdges)
        if (uniform01(rng) < p1)
            R.push_back(e);

    if (R.empty()) return {0.0, {}};

    // -----------------------------------------------------------
    // Stage 2: d(e) = min(deg(u), deg(v)),  d(R) = Σ d(e)
    // -----------------------------------------------------------
    std::vector<double> weights(R.size());
    double dR = 0.0;
    for (size_t i = 0; i < R.size(); ++i) {
        auto [u, v] = R[i];
        double w    = (double)std::min(adj[u].size(), adj[v].size());
        weights[i]  = w;
        dR         += w;
    }
    if (dR == 0.0) return {0.0, {}};

    // -----------------------------------------------------------
    // Stage 3: k = r trials of wedge sampling
    // -----------------------------------------------------------
    std::discrete_distribution<size_t> edgeDist(weights.begin(), weights.end());
    size_t k = r;  // number of trials = r per paper
    size_t X = 0;

    for (size_t i = 0; i < k; ++i) {
        size_t idx  = edgeDist(rng);
        auto [a, b] = R[idx];

        int u = (adj[a].size() <= adj[b].size()) ? a : b;
        int v = (u == a) ? b : a;

        const auto& nu = adj[u];
        size_t wIdx    = std::uniform_int_distribution<size_t>(0, nu.size()-1)(rng);
        auto it        = nu.begin();
        std::advance(it, wIdx);
        int w = *it;

        if (w != v && adj[v].count(w))
            ++X;
    }

    // -----------------------------------------------------------
    // Estimator: T̂ = (X/k) * d(R) / p1
    // -----------------------------------------------------------
    double T_hat = ((double)X / (double)k) * dR / p1;

    TriangleCountingStats stats;
    stats.total_weight = dR;
    stats.r            = r;

    return {T_hat, stats};
}
