#include "ExactArboricity.h"
#include <ranges>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>

// External progress callback defined in the worker
extern std::function<void(const std::string&)> g_progressCallback;

ExactArboricity::MaxFlowSolver::MaxFlowSolver(int n)
    : n(n), adj(n), height(n), count(2 * n, 0), excess(n, 0.0) {}

void ExactArboricity::MaxFlowSolver::addEdge(int u, int v, double cap) {
    adj[u].push_back({v, static_cast<int>(adj[v].size()), cap});
    adj[v].push_back({u, static_cast<int>(adj[u].size()) - 1, 0.0});
}

double ExactArboricity::MaxFlowSolver::solve(int s, int t) {
    std::ranges::fill(height, 0);
    std::ranges::fill(excess, 0.0);
    std::ranges::fill(count, 0);

    height[s] = n;
    count[0] = n - 1;
    count[n] = 1;

    std::vector<size_t> current_edge(n, 0);
    std::vector<int> active_nodes;

    auto push = [&](int u, Edge& e) {
        double delta = std::min(excess[u], e.cap);
        if (delta <= 1e-9 || height[u] <= height[e.to]) return;

        e.cap -= delta;
        adj[e.to][e.rev].cap += delta;
        excess[u] -= delta;
        excess[e.to] += delta;

        if (e.to != s && e.to != t && excess[e.to] <= delta) {
            active_nodes.push_back(e.to);
        }
    };

    auto relabel = [&](int u) {
        int min_h = 2 * n;
        for (const auto& e : adj[u]) {
            if (e.cap > 1e-9) min_h = std::min(min_h, height[e.to]);
        }
        if (min_h < 2 * n) {
            int old_h = height[u];
            if (--count[old_h] == 0 && old_h < n) {
                for (int i = 0; i < n; ++i) {
                    if (height[i] > old_h && height[i] < n) {
                        count[height[i]]--;
                        height[i] = n;
                        count[n]++;
                    }
                }
            } else {
                height[u] = min_h + 1;
                count[height[u]]++;
            }
        }
    };

    for (auto& e : adj[s]) {
        double d = e.cap;
        if (d > 0) {
            e.cap = 0;
            adj[e.to][e.rev].cap += d;
            excess[s] -= d;
            excess[e.to] += d;
            if (e.to != s && e.to != t) active_nodes.push_back(e.to);
        }
    }

    while (!active_nodes.empty()) {
        int u = active_nodes.back();
        active_nodes.pop_back();

        while (excess[u] > 1e-9 && height[u] < n) {
            if (current_edge[u] < adj[u].size()) {
                push(u, adj[u][current_edge[u]]);
                if (excess[u] > 1e-9) current_edge[u]++;
            } else {
                relabel(u);
                current_edge[u] = 0;
            }
        }
    }

    return excess[t];
}

ArboricityOutput ExactArboricity::compute(const std::vector<std::pair<int, int>>& edges) {
    if (edges.empty()) return {0.0, 0, 0};

    if (g_progressCallback) g_progressCallback("Mapping vertex IDs...");

    std::unordered_map<int, int> id_map;
    for (const auto& [u, v] : edges) {
        if (!id_map.contains(u)) id_map[u] = (int)id_map.size();
        if (!id_map.contains(v)) id_map[v] = (int)id_map.size();
    }

    const int n = (int)id_map.size();
    const int m = (int)edges.size();
    int low = 1, high = n, best_k = n;

    if (g_progressCallback) {
        g_progressCallback("Nodes: " + std::to_string(n) + " | Edges: " + std::to_string(m));
        g_progressCallback("Beginning Nash-Williams Forest Cover Search...");
    }

    int iter = 1;
    while (low <= high) {
        int k = low + (high - low) / 2;

        if (g_progressCallback) {
            g_progressCallback("Testing K=" + std::to_string(k) + " (Forest limit)");
        }

        int s = m + n, t = m + n + 1;
        MaxFlowSolver solver(m + n + 2);

        // Map Edges to Vertices
        for (const auto& [i, edge] : std::views::enumerate(edges)) {
            int u_idx = id_map[edge.first];
            int v_idx = id_map[edge.second];
            solver.addEdge(s, (int)i, 1.0);
            solver.addEdge((int)i, m + u_idx, 1.0);
            solver.addEdge((int)i, m + v_idx, 1.0);
        }

        // Capacity constraint for k-forests
        for (int v_idx : std::views::iota(0, n)) {
            solver.addEdge(m + v_idx, t, (double)k);
        }

        double max_flow = solver.solve(s, t);

        if (std::abs(max_flow - m) < 1e-7) {
            if (g_progressCallback) g_progressCallback(" -> SUFFICIENT (k=" + std::to_string(k) + ")");
            best_k = k;
            high = k - 1;
        } else {
            if (g_progressCallback) g_progressCallback(" -> INSUFFICIENT (k=" + std::to_string(k) + ")");
            low = k + 1;
        }
    }

    return { (double)best_k, best_k, n };
}