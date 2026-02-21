#include "ExactArboricity.h"
#include <cmath>
#include <queue>
#include <algorithm>
#include <ranges>
#include <unordered_map>

ExactArboricity::MaxFlowSolver::MaxFlowSolver(int n) 
    : n(n), adj(n), height(n), count(2 * n), excess(n) {}

void ExactArboricity::MaxFlowSolver::addEdge(int u, int v, double cap) {
    adj[u].push_back({v, static_cast<int>(adj[v].size()), cap});
    adj[v].push_back({u, static_cast<int>(adj[u].size()) - 1, 0.0});
}

double ExactArboricity::MaxFlowSolver::solve(int s, int t) {
    std::ranges::fill(height, 0);
    height[s] = n;
    std::ranges::fill(count, 0);
    count[0] = n - 1;
    count[n] = 1;
    std::ranges::fill(excess, 0);
    
    std::vector<bool> active(n, false);
    std::queue<int> q;

    std::ranges::for_each(adj[s], [&](auto& e) {
        double pushed = e.cap;
        e.cap -= pushed;
        adj[e.to][e.rev].cap += pushed;
        excess[s] -= pushed;
        excess[e.to] += pushed;
        if (e.to != s && e.to != t && !active[e.to]) {
            q.push(e.to);
            active[e.to] = true;
        }
    });

    while (!q.empty()) {
        int u = q.front(); 
        q.pop(); 
        active[u] = false;

        for (auto& e : adj[u]) {
            if (e.cap > 1e-9 && height[u] == height[e.to] + 1) {
                double pushed = std::min(excess[u], e.cap);
                e.cap -= pushed;
                adj[e.to][e.rev].cap += pushed;
                excess[u] -= pushed;
                excess[e.to] += pushed;
                if (e.to != s && e.to != t && !active[e.to]) {
                    q.push(e.to); 
                    active[e.to] = true;
                }
                if (excess[u] <= 1e-9) break;
            }
        }

        if (excess[u] > 1e-9) {
            int old_h = height[u];
            
            auto reachable_heights = adj[u] 
                | std::views::filter([](const auto& e) { return e.cap > 1e-9; })
                | std::views::transform([&](const auto& e) { return height[e.to]; });
            
            int min_h = 2 * n;
            if (!reachable_heights.empty()) {
                min_h = std::ranges::min(reachable_heights);
            }

            height[u] = min_h + 1;
            count[height[u]]++;
            
            if (--count[old_h] == 0 && old_h < n) {
                for (auto i : std::views::iota(0, n)) {
                    if (i != s && i != t && height[i] > old_h && height[i] <= n) {
                        count[height[i]]--;
                        height[i] = n + 1;
                    }
                }
            }
            q.push(u); 
            active[u] = true;
        }
    }
    return excess[t];
}

ArboricityOutput ExactArboricity::compute(const std::vector<std::pair<int, int>>& edges) {
    if (edges.empty()) return {0.0, 0, 0};

    std::unordered_map<int, int> id_map;
    std::ranges::for_each(edges, [&](const auto& e) {
        if (!id_map.contains(e.first)) 
            id_map[e.first] = static_cast<int>(id_map.size());
        if (!id_map.contains(e.second)) 
            id_map[e.second] = static_cast<int>(id_map.size());
    });

    int n = static_cast<int>(id_map.size());
    int m = static_cast<int>(edges.size());
    
    std::vector<int> degree(n, 0);
    std::ranges::for_each(edges, [&](const auto& e) {
        degree[id_map[e.first]]++;
        degree[id_map[e.second]]++;
    });

    double low = 0, high = m;
    double threshold = 1.0 / (static_cast<double>(n) * (n - 1));

    while (high - low > threshold) {
        double g = (low + high) / 2.0;
        int s = n, t = n + 1;
        MaxFlowSolver solver(n + 2);

        for (int i : std::views::iota(0, n)) {
            solver.addEdge(s, i, m);
            solver.addEdge(i, t, m + 2.0 * g - degree[i]);
        }

        std::ranges::for_each(edges, [&](const auto& e) {
            solver.addEdge(id_map[e.first], id_map[e.second], 1.0);
            solver.addEdge(id_map[e.second], id_map[e.first], 1.0);
        });

        if ((n * m - solver.solve(s, t)) > 1e-7) 
            low = g;
        else 
            high = g;
    }

    return {2.0 * low, static_cast<int>(std::ceil(2.0 * low)), 0};
}
