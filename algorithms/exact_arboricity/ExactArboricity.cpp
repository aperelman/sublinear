#include "ExactArboricity.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>

ExactArboricity::MaxFlowSolver::MaxFlowSolver(int n)
    : n(n), adj(n), height(n), count(2 * n, 0), excess(n, 0.0) {}

void ExactArboricity::MaxFlowSolver::addEdge(int u, int v, double cap) {
    adj[u].push_back({v, cap, adj[v].size()});
    adj[v].push_back({u, 0.0, adj[u].size() - 1});
}

double ExactArboricity::MaxFlowSolver::solve(int s, int t) {
    std::fill(height.begin(), height.end(), 0);
    std::fill(excess.begin(), excess.end(), 0.0);
    std::fill(count.begin(), count.end(), 0);
    height[s] = n;
    count[0] = n - 1;
    count[n] = 1;
    std::vector<size_t> curr_edge(n, 0);
    std::vector<int> active;

    auto push = [&](int u, Edge& e) {
        double d = std::min(excess[u], e.cap);
        if (d <= 1e-9 || height[u] <= height[e.to]) return;
        e.cap -= d;
        adj[e.to][e.rev].cap += d;
        if (excess[e.to] <= 1e-9 && e.to != s && e.to != t) active.push_back(e.to);
        excess[u] -= d;
        excess[e.to] += d;
    };

    for (auto& e : adj[s]) {
        double d = e.cap;
        e.cap -= d;
        adj[e.to][e.rev].cap += d;
        excess[e.to] += d;
        if (e.to != s && e.to != t) active.push_back(e.to);
    }

    while (!active.empty()) {
        int u = active.back();
        active.pop_back();
        while (excess[u] > 1e-9) {
            if (curr_edge[u] < (int)adj[u].size()) {
                auto& e = adj[u][curr_edge[u]];
                if (e.cap > 1e-9 && height[u] == height[e.to] + 1) push(u, e);
                else curr_edge[u]++;
            } else {
                int old_h = height[u];
                int min_h = 2 * n;
                for (auto& e : adj[u]) if (e.cap > 1e-9) min_h = std::min(min_h, height[e.to] + 1);
                height[u] = (min_h == 2 * n) ? old_h : min_h;
                count[old_h]--;
                if (count[old_h] == 0 && old_h < n) {
                    for (int i = 0; i < n; ++i) {
                        if (i != s && i != t && height[i] > old_h && height[i] < n) {
                            count[height[i]]--;
                            height[i] = n;
                            count[n]++;
                        }
                    }
                }
                count[height[u]]++;
                curr_edge[u] = 0;
            }
        }
    }
    return excess[t];
}

ArboricityOutput ExactArboricity::compute(const std::vector<std::pair<int, int>>& edges, int degeneracy, LogFn log) {
    if (edges.empty()) return {0.0, 0, 0};
    auto lg = [&](const std::string& m) { if (log) log(m); };

    std::unordered_map<int, int> id_map;
    for (const auto& e : edges) {
        if (id_map.find(e.first) == id_map.end()) id_map[e.first] = (int)id_map.size();
        if (id_map.find(e.second) == id_map.end()) id_map[e.second] = (int)id_map.size();
    }

    const int n = (int)id_map.size();
    const int m = (int)edges.size();
    int low = (n > 1) ? std::ceil((double)m / (n - 1)) : 0;
    int high = (degeneracy > 0) ? degeneracy : n;
    int best_k = high;

    while (low <= high) {
        int k = low + (high - low) / 2;
        lg("Testing K=" + std::to_string(k) + " | Range: [" + std::to_string(low) + "..." + std::to_string(high) + "]");
        MaxFlowSolver solver(n + 2);
        int s = n, t = n + 1;

        for (const auto& e : edges) {
            solver.addEdge(s, id_map[e.first], 1.0);
            solver.addEdge(id_map[e.first], id_map[e.second], 1e9);
        }
        for (int i = 0; i < n; ++i) solver.addEdge(i, t, (double)k);

        if (std::abs(solver.solve(s, t) - m) < 1e-7) {
            lg(" -> Valid.");
            best_k = k;
            high = k - 1;
        } else {
            lg(" -> Invalid.");
            low = k + 1;
        }
    }
    return {(double)best_k, best_k, n};
}