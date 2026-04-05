#ifndef ARBORICITY_SOLVER_H
#define ARBORICITY_SOLVER_H

#include <vector>
#include <utility>
#include <functional>
#include <unordered_map>
#include <string>

/**
 * @brief High-performance exact arboricity solver using Nash-Williams theorem.
 *
 * Algorithm:
 *   Binary search on k in [lo, hi], where each candidate k is verified by
 *   constructing a 3-layer flow network and running Dinic's max-flow algorithm.
 *   The graph has arboricity k iff max-flow == |E|.
 *
 * Performance features (no threads required):
 *   - Integer capacities (exact arithmetic, no epsilon comparisons)
 *   - Iterative DFS with advance-pointer (no recursion, no stack overflow)
 *   - Warm-start: flow network persists across binary search steps;
 *     only sink capacities are patched between iterations (O(V) vs O(V+E))
 *   - Flow reset only when moving to a larger k (feasibility lost)
 *   - Tight upper bound: min(degeneracy, ceil(maxDegree/2)+1)
 *   - Fast path: tests lo before entering binary search
 *   - Densest subgraph extraction for UI highlighting
 */
class ArboricitySolver {
public:
    using LogFn    = std::function<void(const std::string&)>;
    using ProgressFn = std::function<void(int currentK, int lo, int hi)>;

    /**
     * @param numNodes  Total number of vertices (node IDs are 0-based).
     */
    explicit ArboricitySolver(int numNodes);

    /** Add an undirected edge. Call before computeExact(). */
    void addEdge(int u, int v);

    /**
     * Compute exact arboricity.
     *
     * @param onProgress  Optional callback fired each binary-search step.
     * @param log         Optional logger for diagnostic messages.
     * @return            Exact arboricity (integer >= 0).
     */
    int computeExact(ProgressFn onProgress = nullptr,
                     LogFn      log        = nullptr);

    /**
     * Returns nodes of the densest subgraph found during the last
     * infeasible partition check. Useful for UI highlighting.
     */
    const std::vector<int>& getDensestSubgraph() const;

private:
    // -----------------------------------------------------------------------
    // Flow network (integer capacities for exact arithmetic)
    // -----------------------------------------------------------------------
    struct FlowEdge {
        int  to;
        long cap;   // residual capacity
        int  rev;   // index of reverse edge in adj[to]
    };

    using Graph = std::vector<std::vector<FlowEdge>>;

    // Node layout inside the flow network:
    //   0          = super-source
    //   1 .. M     = edge-nodes  (one per original edge)
    //   M+1 .. M+N = vertex-nodes
    //   M+N+1      = super-sink
    int m_source{0};
    int m_sink{0};
    int m_flowN{0};   // total nodes in flow network

    Graph m_graph;    // persistent across binary-search iterations
    int   m_currentK{-1};

    // -----------------------------------------------------------------------
    // Dinic's algorithm internals
    // -----------------------------------------------------------------------
    std::vector<int>    m_level;
    std::vector<int>    m_ptr;   // advance pointer per node (int, not size_t)

    bool bfs();
    long runMaxFlow();   // iterative DFS embedded inside

    // -----------------------------------------------------------------------
    // Network construction / warm-start
    // -----------------------------------------------------------------------
    void buildNetwork(int k);
    void patchSinkCapacity(int oldK, int newK);
    void resetFlow();

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------
    int  computeDegeneracy() const;
    int  computeUpperBound(int degeneracy) const;
    void extractDensestSubgraph();

    void addFlowEdge(int u, int v, long cap);

    // -----------------------------------------------------------------------
    // Graph data
    // -----------------------------------------------------------------------
    int m_numNodes;
    std::vector<std::pair<int,int>> m_edges;
    std::vector<int>                m_densestSubgraph;
};

#endif // ARBORICITY_SOLVER_H
