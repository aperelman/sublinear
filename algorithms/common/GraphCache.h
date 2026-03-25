#ifndef GRAPH_CACHE_H
#define GRAPH_CACHE_H

#include <string>
#include <vector>
#include <utility>
#include <functional>
#include <mutex>
#include <atomic>

/**
 * @brief Thread-safe Graph Cache
 * Stores the edge list and metadata of the currently loaded graph.
 * Prevents re-parsing the same file multiple times during a 3-step analysis.
 */
class GraphCache {
public:
    GraphCache() : m_nodeCount(0), m_edgeCount(0) {}
    ~GraphCache() = default;

    // Disallow copying to prevent mutex/pointer issues
    GraphCache(const GraphCache&) = delete;
    GraphCache& operator=(const GraphCache&) = delete;

    /**
     * @brief Ensures a graph is loaded into memory.
     * @param path The filesystem path to the normalized graph file.
     * @param loader A callback function to log status messages back to the UI.
     * @return true if the graph is ready in memory, false if loading failed.
     */
    bool ensure(const std::string& path, std::function<void(const std::string&)> loader);

    /**
     * @brief Clears the current cache to free memory.
     */
    void invalidate();

    // Data Accessors (Thread-Safe after ensure() returns true)
    const std::vector<std::pair<int, int>>& edges() const { return m_edges; }
    int64_t nodeCount() const { return m_nodeCount; }
    int64_t edgeCount() const { return m_edgeCount; }
    std::string currentPath() const { return m_currentPath; }

private:
    std::string m_currentPath;
    std::vector<std::pair<int, int>> m_edges;
    int64_t m_nodeCount;
    int64_t m_edgeCount;

    // Mutex protects the 'ensure' and 'invalidate' logic from concurrent thread access
    mutable std::mutex m_mutex;
};

#endif // GRAPH_CACHE_H