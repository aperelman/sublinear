#include "GraphCache.h"
#include <fstream>
#include <sstream>
#include <chrono>

// We use SNAP for loading to stay consistent with the rest of the pipeline.
// SNAP handles format detection, comment skipping, etc.
// We include it here only — not in the header — to avoid AUTOMOC pollution.
#include <Snap.h>

void GraphCache::invalidate() {
    m_loaded    = false;
    m_filePath.clear();
    m_nodeCount = 0;
    m_edgeCount = 0;
    m_edges.clear();
    m_adj.clear();
    m_idMap.clear();
    m_originalIds.clear();
}

bool GraphCache::ensure(const std::string& filePath, LogFn log) {
    auto lg = [&](const std::string& msg) { if (log) log(msg); };

    // Already cached for this path — instant return
    if (m_loaded && m_filePath == filePath) {
        lg("Using cached graph: " + filePath +
           " (n=" + std::to_string(m_nodeCount) +
           " m=" + std::to_string(m_edgeCount) + ")");
        return true;
    }

    // Need to load
    invalidate();

    lg("--- Phase: Graph Loading ---");
    auto t0 = std::chrono::high_resolution_clock::now();

    PUNGraph Graph;
    try {
        Graph = TSnap::LoadEdgeList<PUNGraph>(filePath.c_str(), 0, 1);
    } catch (...) {
        lg("ERROR: Failed to load graph from " + filePath);
        return false;
    }

    if (Graph.Empty() || Graph->GetNodes() == 0) {
        lg("ERROR: Empty graph loaded from " + filePath);
        return false;
    }

    m_nodeCount = Graph->GetNodes();
    m_edgeCount = Graph->GetEdges();

    auto elapsed = [&]() {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(now - t0).count();
    };

    lg("Loaded " + std::to_string(m_nodeCount) + " nodes, " +
       std::to_string(m_edgeCount) + " edges in " +
       std::to_string((int)elapsed()) + " ms.");

    // --- Build ID mapping: original SNAP IDs -> internal [0, n) ---
    lg("--- Phase: Building Graph Structures ---");
    auto tb = std::chrono::high_resolution_clock::now();

    m_idMap.reserve(m_nodeCount);
    m_originalIds.reserve(m_nodeCount);

    int internalId = 0;
    for (TUNGraph::TNodeI NI = Graph->BegNI(); NI < Graph->EndNI(); NI++) {
        int origId = NI.GetId();
        m_idMap[origId] = internalId;
        m_originalIds.push_back(origId);
        ++internalId;
    }

    // --- Build adjacency list ---
    m_adj.resize(m_nodeCount);
    m_edges.reserve(m_edgeCount);

    for (TUNGraph::TNodeI NI = Graph->BegNI(); NI < Graph->EndNI(); NI++) {
        int u = m_idMap[NI.GetId()];
        for (int i = 0; i < NI.GetOutDeg(); ++i) {
            int v = m_idMap[NI.GetOutNId(i)];
            if (u < v) {
                m_edges.push_back({u, v});
            }
            m_adj[u].push_back(v);
        }
    }

    // Sort adjacency lists for binary search (hasEdge)
    for (auto& nbrs : m_adj) {
        std::sort(nbrs.begin(), nbrs.end());
    }

    auto buildMs = std::chrono::duration<double, std::milli>(
        std::chrono::high_resolution_clock::now() - tb).count();

    lg("Graph structures built in " + std::to_string((int)buildMs) + " ms. " +
       "Edges: " + std::to_string(m_edges.size()));

    m_filePath = filePath;
    m_loaded   = true;
    return true;
}
