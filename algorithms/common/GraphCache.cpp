#include "GraphCache.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <mutex>
#include <unordered_set>

/**
 * @brief Thread-safe implementation of the Graph Cache.
 */
bool GraphCache::ensure(const std::string& path, std::function<void(const std::string&)> loader) {
    // 1. Thread Safety: Lock the mutex so multiple worker threads
    // don't try to load/modify the same map/vector simultaneously.
    std::lock_guard<std::mutex> lock(m_mutex);

    // 2. Cache Check: If this file is already loaded, skip the parsing phase.
    if (m_currentPath == path && !m_edges.empty()) {
        if (loader) loader("Using cached graph data from memory.");
        return true;
    }

    // 3. Cleanup: Invalidate old data before loading new data.
    m_edges.clear();
    m_nodeCount = 0;
    m_edgeCount = 0;
    m_currentPath = "";

    if (loader) loader("Parsing graph file into memory...");

    std::ifstream infile(path);
    if (!infile.is_open()) {
        if (loader) loader("Error: Could not open file for caching.");
        return false;
    }

    // 4. Parsing: Read the edge list (Expected format: "source target")
    // Using strtol instead of stringstream for performance (no heap alloc per line).
    char line[256];
    int maxNodeId = -1;

    while (infile.getline(line, sizeof(line))) {
        if (line[0] == '#' || line[0] == '\0') continue;
        char* end;
        long long u = std::strtoll(line, &end, 10);
        if (end == line) continue;  // no number found
        long long v = std::strtoll(end, &end, 10);
        if (end == line) continue;  // no second number
        m_edges.emplace_back((int)u, (int)v);
        if (u > maxNodeId) maxNodeId = (int)u;
        if (v > maxNodeId) maxNodeId = (int)v;
    }

    // Normalize to undirected: canonicalize (u,v) -> (min,max) and deduplicate.
    // This handles directed graphs (e.g. SNAP) that store both (u,v) and (v,u).
    for (auto& e : m_edges)
        if (e.first > e.second) std::swap(e.first, e.second);
    std::sort(m_edges.begin(), m_edges.end());
    m_edges.erase(std::unique(m_edges.begin(), m_edges.end()), m_edges.end());

    m_edgeCount = static_cast<int64_t>(m_edges.size());

    // Count distinct node IDs — do NOT use maxNodeId+1 since SNAP node IDs
    // are not contiguous (e.g. as-Caida has IDs up to 65535 but only 26475 nodes).
    // TriangleCounting and ArboricitySolver both remap internally, but nodeCount
    // is reported in the UI and used for flow network sizing — must be accurate.
    {
        std::unordered_set<int> nodeSet;
        nodeSet.reserve(m_edges.size() * 2);
        for (const auto& e : m_edges) {
            nodeSet.insert(e.first);
            nodeSet.insert(e.second);
        }
        m_nodeCount = static_cast<int64_t>(nodeSet.size());
    }
    m_currentPath = path;

    if (loader) {
        loader(std::string("Cache loaded: ") + std::to_string(m_nodeCount) +
               " nodes, " + std::to_string(m_edgeCount) + " edges.");
    }

    return true;
}

void GraphCache::invalidate() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_edges.clear();
    m_edges.shrink_to_fit(); // Release memory back to OS
    m_nodeCount = 0;
    m_edgeCount = 0;
    m_currentPath = "";
}