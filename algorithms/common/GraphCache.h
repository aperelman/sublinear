#ifndef GRAPH_CACHE_H
#define GRAPH_CACHE_H

#include <vector>
#include <utility>
#include <string>
#include <functional>
#include <cstdint>
#include <unordered_map>
#include <algorithm>

/**
 * GraphCache — loads a graph file once and caches all derived structures.
 * Avoids repeated file I/O across pipeline steps (arboricity, triangle counting, sampling).
 */
class GraphCache {
public:
    using LogFn = std::function<void(const std::string&)>;

    GraphCache() = default;

    // Ensure the graph for filePath is loaded. Returns false on error.
    // If already cached for this exact path, returns true immediately (no I/O).
    bool ensure(const std::string& filePath, LogFn log = nullptr);

    // Invalidate cache
    void invalidate();

    bool        isLoaded()  const { return m_loaded; }
    std::string filePath()  const { return m_filePath; }
    int64_t     nodeCount() const { return m_nodeCount; }
    int64_t     edgeCount() const { return m_edgeCount; }

    // Edge list with internal IDs [0, n), u < v guaranteed
    const std::vector<std::pair<int,int>>& edges() const { return m_edges; }

    // Adjacency list with internal IDs, each list sorted
    const std::vector<std::vector<int>>& adj() const { return m_adj; }

    // Neighbor access by internal ID
    const std::vector<int>& neighbors(int u) const { return m_adj[u]; }
    int degree(int u) const { return (int)m_adj[u].size(); }

    // Edge existence check (internal IDs)
    bool hasEdge(int u, int v) const {
        if (u < 0 || u >= (int)m_adj.size()) return false;
        return std::binary_search(m_adj[u].begin(), m_adj[u].end(), v);
    }

    // Convert original node ID to internal ID (-1 if not found)
    int toInternal(int originalId) const {
        auto it = m_idMap.find(originalId);
        return (it != m_idMap.end()) ? it->second : -1;
    }

    // Convert internal ID to original ID
    int toOriginal(int internalId) const {
        if (internalId < 0 || internalId >= (int)m_originalIds.size()) return -1;
        return m_originalIds[internalId];
    }

private:
    bool        m_loaded    = false;
    std::string m_filePath;
    int64_t     m_nodeCount = 0;
    int64_t     m_edgeCount = 0;

    std::vector<std::pair<int,int>> m_edges;
    std::vector<std::vector<int>>   m_adj;

    std::unordered_map<int,int> m_idMap;      // original -> internal
    std::vector<int>            m_originalIds; // internal -> original
};

#endif // GRAPH_CACHE_H
