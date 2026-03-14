#include "exact_arboricity/ArboricitySolver.h"
#include <cmath>

ArboricitySolver::ArboricitySolver(int numNodes) : m_numNodes(numNodes) {}

void ArboricitySolver::addEdge(int u, int v) {
    m_edges.push_back({u, v});
}

std::vector<int> ArboricitySolver::getDensestSubgraph() const {
    return m_criticalNodes;
}

/**
 * The core of the Nash-Williams check.
 * We build a bipartite-style flow network:
 * Source -> [Edge Nodes] -> [Vertex Nodes] -> Sink
 */
bool ArboricitySolver::canPartition(int k) {
    if (m_numNodes <= 1) return true;
    if (k <= 0) return false;

    // Node Indexing:
    // 0: Source
    // 1 to m_edges.size(): Edge Nodes
    // m_edges.size() + 1 to m_edges.size() + m_numNodes: Vertex Nodes
    // m_edges.size() + m_numNodes + 1: Sink
    int source = 0;
    int edgeNodeOffset = 1;
    int vertexNodeOffset = static_cast<int>(m_edges.size()) + 1;
    int sink = vertexNodeOffset + m_numNodes;

    std::vector<std::vector<FlowEdge>> adj(sink + 1);

    auto add_flow_edge = [&](int from, int to, double cap) {
        adj[from].push_back({to, cap, 0, adj[to].size()});
        adj[to].push_back({from, 0, 0, adj[from].size() - 1});
    };

    // 1. Source to Edge Nodes: Each graph edge has capacity 1
    for (size_t i = 0; i < m_edges.size(); ++i) {
        add_flow_edge(source, edgeNodeOffset + i, 1.0);

        // 2. Edge Nodes to their incident Vertex Nodes: Infinite capacity
        // This forces the flow to choose which vertex the edge "belongs" to
        add_flow_edge(edgeNodeOffset + i, vertexNodeOffset + m_edges[i].first, std::numeric_limits<double>::infinity());
        add_flow_edge(edgeNodeOffset + i, vertexNodeOffset + m_edges[i].second, std::numeric_limits<double>::infinity());
    }

    // 3. Vertex Nodes to Sink: Capacity k (the arboricity threshold)
    // According to Nash-Williams, a subgraph H can have at most k(|V(H)| - 1) edges.
    // In flow terms, we check if we can orient edges so no vertex has in-degree > k.
    for (int i = 0; i < m_numNodes; ++i) {
        add_flow_edge(vertexNodeOffset + i, sink, static_cast<double>(k));
    }

    double maxFlow = runMaxFlow(source, sink, adj);

    // If we saturated all edges (flow == |E|), the graph can be partitioned into k forests.
    bool possible = (std::abs(maxFlow - static_cast<double>(m_edges.size())) < 1e-7);

    // If NOT possible, find the Min-Cut to identify the violating dense subgraph
    if (!possible) {
        m_criticalNodes.clear();
        std::vector<int> level(sink + 1, -1);
        bfs(source, sink, adj, level); // Last BFS to find reachability in residual graph
        for (int i = 0; i < m_numNodes; ++i