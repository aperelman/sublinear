#ifndef ARBORICITY_SOLVER_H
#define ARBORICITY_SOLVER_H

#include <vector>
#include <utility>
#include <queue>
#include <limits>
#include <algorithm>
#include <functional>

/**
 * @brief High-performance solver for Exact Arboricity using Nash-Williams theorem.
 * * This class calculates the minimum number of forests needed to cover the edges of a graph.
 * It utilizes a binary search over potential arboricity values (k), verified by
 * constructing a flow network and executing Dinic's Max-Flow algorithm.
 */
class ArboricitySolver {
public:
    /**
     * @brief Construct a new Arboricity Solver
     * @param numNodes The total number of vertices in the graph (from SNAP or local model).
     */
    explicit ArboricitySolver(int numNodes);

    /**
     * @brief Adds an undirected edge to the internal representation.
     * @param u Source node index.
     * @param v Destination node index.
     */
    void addEdge(int u, int v);

    /**
     * @brief Computes the exact arboricity of the graph.
     * @param onProgress Optional callback: void(int currentK, int low, int high).
     * @return The exact arboricity (integer).
     */
    int computeExact(std::function<void(int, int, int)> onProgress = nullptr);

    /**
     * @brief Returns the nodes of the densest subgraph found during the last failed partition check.
     * Useful for UI highlighting in the GraphAnalyzer.
     * @return std::vector<int> List of node indices.
     */
    std::vector<int> getDensestSubgraph() const;

private:
    // Internal structure for Dinic's flow network edges
    struct FlowEdge {
        int to;
        double capacity;
        double flow;
        size_t rev; // Index of the reverse edge in adj[to]
    };

    /**
     * @brief Checks if the graph can be partitioned into k forests.
     * Implements the flow-based density check: |E(H)| <= k(|V(H)| - 1).
     */
    bool canPartition(int k);

    // --- Dinic's Algorithm Core Helpers ---
    double runMaxFlow(int s, int t, std::vector<std::vector<FlowEdge>>& adj);
    bool bfs(int s, int t, const std::vector<std::vector<FlowEdge>>& adj, std::vector<int>& level);
    double dfs(int v, int t, double pushed, std::vector<int>& level, std::vector<size_t>& ptr, std::vector<std::vector<FlowEdge>>& adj);

    int m_numNodes;
    std::vector<std::pair<int, int>> m_edges;
    std::vector<int> m_criticalNodes; // Stores the Min-Cut result (densest subgraph).
};

#endif // ARBORICITY_SOLVER_H