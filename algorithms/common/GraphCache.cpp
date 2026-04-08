#include "GraphCache.h"
#include <fstream>
#include <algorithm>
#include <mutex>
#include <unordered_set>

bool GraphCache::ensure(const std::string& path, std::function<void(const std::string&)> loader) {
    // Fast check under lock — release before calling loader (loader invokes Qt signals)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_currentPath == path && !m_edges.empty()) {
            lock.unlock();
            if (loader) loader("Using cached graph data from memory.");
            return true;
        }
        // Reset state before releasing lock
        m_edges.clear();
        m_nodeCount = 0;
        m_edgeCount = 0;
        m_currentPath = "";
    }

    // Log and do file I/O outside the lock — this is the slow part
    if (loader) loader("Parsing graph file into memory...");

    std::ifstream infile(path);
    if (!infile.is_open()) {
        if (loader) loader("Error: Could not open file for caching.");
        return false;
    }

    std::vector<std::pair<int,int>> edges;
    edges.reserve(1 << 20);

    char line[256];
    while (infile.getline(line, sizeof(line))) {
        if (line[0] == '#' || line[0] == '\0') continue;
        char* end;
        long long u = std::strtoll(line, &end, 10);
        if (end == line) continue;
        long long v = std::strtoll(end, nullptr, 10);
        edges.emplace_back((int)u, (int)v);
    }

    // Normalize to undirected and deduplicate
    for (auto& e : edges)
        if (e.first > e.second) std::swap(e.first, e.second);
    std::sort(edges.begin(), edges.end());
    edges.erase(std::unique(edges.begin(), edges.end()), edges.end());

    int64_t edgeCount = static_cast<int64_t>(edges.size());

    std::unordered_set<int> nodeSet;
    nodeSet.reserve(edges.size() * 2);
    for (const auto& e : edges) {
        nodeSet.insert(e.first);
        nodeSet.insert(e.second);
    }
    int64_t nodeCount = static_cast<int64_t>(nodeSet.size());

    // Store results under lock
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_edges     = std::move(edges);
        m_edgeCount = edgeCount;
        m_nodeCount = nodeCount;
        m_currentPath = path;
    }

    if (loader) {
        loader(std::string("Cache loaded: ") + std::to_string(nodeCount) +
               " nodes, " + std::to_string(edgeCount) + " edges.");
    }

    return true;
}

void GraphCache::invalidate() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_edges.clear();
    m_edges.shrink_to_fit();
    m_nodeCount = 0;
    m_edgeCount = 0;
    m_currentPath = "";
}
