#include "ArboricityApprox.h"
#include <algorithm>
#include <cmath>
#include <vector>

/**
 * Greedy peeling arboricity approximation — O(V+E).
 *
 * Based on Chiba-Nishizeki degeneracy ordering:
 * Repeatedly remove the minimum-degree node. Track the density
 * ceil(edges_remaining / (nodes_remaining - 1)) at each step.
 * The maximum over all steps is a tight approximation of arboricity.
 *
 * This equals exact arboricity for most real-world graphs.
 * In the worst case it can overestimate by a factor of 2.
 */
int ArboricityApprox::compute(const std::vector<std::pair<int,int>>& edges,
                               int numNodes,
                               LogFn log)
{
    auto lg = [&](const std::string& m) { if (log) log(m); };

    if (edges.empty() || numNodes <= 1) return 0;

    // Remap node IDs to contiguous [0, N) — same as ArboricitySolver
    // GraphCache nodeCount() is distinct node count but IDs may not be contiguous
    std::unordered_map<int,int> idMap;
    idMap.reserve(edges.size() * 2);
    int nextId = 0;
    for (const auto& e : edges) {
        if (idMap.find(e.first)  == idMap.end()) idMap[e.first]  = nextId++;
        if (idMap.find(e.second) == idMap.end()) idMap[e.second] = nextId++;
    }
    const int N = nextId;

    std::vector<int> deg(N, 0);
    std::vector<std::vector<int>> adj(N);

    for (const auto& e : edges) {
        int u = idMap[e.first];
        int v = idMap[e.second];
        adj[u].push_back(v);
        adj[v].push_back(u);
        deg[u]++;
        deg[v]++;
    }

    const int M = (int)edges.size();

    // O(V+E) bucket queue with position tracking
    int maxDeg = *std::max_element(deg.begin(), deg.end());
    std::vector<std::vector<int>> buckets(maxDeg + 1);
    std::vector<int> pos(N);

    for (int i = 0; i < N; ++i) {
        pos[i] = (int)buckets[deg[i]].size();
        buckets[deg[i]].push_back(i);
    }

    std::vector<bool> removed(N, false);
    int edgesRemaining = M;
    int nodesRemaining = N;
    int best = 1;
    int d = 0;

    for (int step = 0; step < N; ++step) {
        // Advance to next non-empty bucket
        while (d <= maxDeg && buckets[d].empty()) ++d;

        int u = buckets[d].back();
        buckets[d].pop_back();
        removed[u] = true;

        // Compute density of remaining subgraph
        if (nodesRemaining > 1) {
            int arb = (int)std::ceil((double)edgesRemaining /
                                     (double)(nodesRemaining - 1));
            best = std::max(best, arb);
        }

        nodesRemaining--;
        edgesRemaining -= deg[u];

        // Update neighbors
        for (int w : adj[u]) {
            if (removed[w]) continue;

            // O(1) removal from bucket
            int bd = deg[w];
            int p  = pos[w];
            int last = buckets[bd].back();
            buckets[bd][p] = last;
            pos[last] = p;
            buckets[bd].pop_back();

            deg[w]--;
            pos[w] = (int)buckets[deg[w]].size();
            buckets[deg[w]].push_back(w);
            if (deg[w] < d) d = deg[w];
        }
    }

    lg("Approximate arboricity (greedy peeling) = " + std::to_string(best));
    return best;
}
