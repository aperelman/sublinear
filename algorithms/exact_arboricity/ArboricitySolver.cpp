#include "ArboricitySolver.h"
#include <cmath>
#include <queue>
#include <limits>

ArboricitySolver::ArboricitySolver(int numNodes) : m_numNodes(numNodes) {}

void ArboricitySolver::addEdge(int u, int v) {
    m_edges.push_back({u, v});
}

std::vector<int> ArboricitySolver::getDensestSubgraph() const {
    return m_criticalNodes;
}

bool ArboricitySolver::bfs(int s, int t,
                            const std::vector<std::vector<FlowEdge>>& adj,
                            std::vector<int>& level) {
    std::fill(level.begin(), level.end(), -1);
    level[s] = 0;
    std::queue<int> q;
    q.push(s);
    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (const auto& e : adj[u]) {
            if (level[e.to] < 0 && e.capacity - e.flow > 1e-9) {
                level[e.to] = level[u] + 1;
                q.push(e.to);
            }
        }
    }
    return level[t] >= 0;
}

double ArboricitySolver::dfs(int v, int t, double pushed,
                              std::vector<int>& level,
                              std::vector<size_t>& ptr,
                              std::vector<std::vector<FlowEdge>>& adj) {
    if (v == t) return pushed;
    for (size_t& i = ptr[v]; i < adj[v].size(); ++i) {
        FlowEdge& e = adj[v][i];
        if (level[e.to] != level[v] + 1) continue;
        double rem = e.capacity - e.flow;
        if (rem < 1e-9) continue;
        double d = dfs(e.to, t, std::min(pushed, rem), level, ptr, adj);
        if (d > 1e-9) {
            e.flow += d;
            adj[e.to][e.rev].flow -= d;
            return d;
        }
    }
    return 0.0;
}

double ArboricitySolver::runMaxFlow(int s, int t,
                                     std::vector<std::vector<FlowEdge>>& adj) {
    double flow = 0.0;
    std::vector<int> level(adj.size());
    while (bfs(s, t, adj, level)) {
        std::vector<size_t> ptr(adj.size(), 0);
        double pushed;
        while ((pushed = dfs(s, t, std::numeric_limits<double>::infinity(),
                              level, ptr, adj)) > 1e-9) {
            flow += pushed;
        }
    }
    return flow;
}

bool ArboricitySolver::canPartition(int k) {
    if (m_numNodes <= 1) return true;
    if (k <= 0) return false;

    int source = 0;
    int edgeNodeOffset = 1;
    int vertexNodeOffset = static_cast<int>(m_edges.size()) + 1;
    int sink = vertexNodeOffset + m_numNodes;

    std::vector<std::vector<FlowEdge>> adj(sink + 1);

    auto add_flow_edge = [&](int from, int to, double cap) {
        adj[from].push_back({to, cap, 0.0, adj[to].size()});
        adj[to].push_back({from, 0.0, 0.0, adj[from].size() - 1});
    };

    for (size_t i = 0; i < m_edges.size(); ++i) {
        add_flow_edge(source, edgeNodeOffset + (int)i, 1.0);
        add_flow_edge(edgeNodeOffset + (int)i,
                      vertexNodeOffset + m_edges[i].first,
                      std::numeric_limits<double>::infinity());
        add_flow_edge(edgeNodeOffset + (int)i,
                      vertexNodeOffset + m_edges[i].second,
                      std::numeric_limits<double>::infinity());
    }

    for (int i = 0; i < m_numNodes; ++i) {
        add_flow_edge(vertexNodeOffset + i, sink, static_cast<double>(k));
    }

    double maxFlow = runMaxFlow(source, sink, adj);
    bool possible = (std::abs(maxFlow - static_cast<double>(m_edges.size())) < 1e-7);

    if (!possible) {
        m_criticalNodes.clear();
        std::vector<int> level(sink + 1, -1);
        bfs(source, sink, adj, level);
        for (int i = 0; i < m_numNodes; ++i) {
            if (level[vertexNodeOffset + i] >= 0) {
                m_criticalNodes.push_back(i);
            }
        }
    }

    return possible;
}

int ArboricitySolver::computeExact(std::function<void(int, int, int)> onProgress) {
    int lo = 1, hi = static_cast<int>(m_edges.size());
    if (hi == 0) return 0;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (onProgress) onProgress(mid, lo, hi);
        if (canPartition(mid)) hi = mid;
        else lo = mid + 1;
    }
    canPartition(lo);
    return lo;
}
