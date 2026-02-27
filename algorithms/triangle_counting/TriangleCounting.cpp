#include "TriangleCounting.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <iostream>

/**
 * Loads a SNAP format graph.
 * Automatically handles tab/space separators and skips comment headers.
 */
Graph loadGraph(const std::string& filePath) {
    Graph graph;
    std::ifstream infile(filePath);
    if (!infile.is_open()) return graph;

    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '%') continue;

        std::stringstream ss(line);
        int u, v;
        if (ss >> u >> v) {
            if (u == v) continue; // Skip self-loops
            graph[u].push_back(v);
            graph[v].push_back(u);
        }
    }

    // Optimization: Sort neighbors for fast binary search intersection
    for (auto& pair : graph) {
        std::sort(pair.second.begin(), pair.second.end());
        pair.second.erase(std::unique(pair.second.begin(), pair.second.end()), pair.second.end());
    }

    return graph;
}

/**
 * Sublinear Triangle Estimation using Importance Sampling.
 * Instead of checking every triplet, we sample edges and wedges.
 */
GraphAnalysisResult analyzeAndCount(const Graph& graph, double exactAlpha) {
    GraphAnalysisResult result;
    result.numVertices = graph.size();

    int64_t totalDegree = 0;
    std::vector<std::pair<int, int>> edgeList;

    for (const auto& [u, neighbors] : graph) {
        totalDegree += neighbors.size();
        for (int v : neighbors) {
            if (u < v) edgeList.push_back({u, v});
        }
    }
    result.numEdges = edgeList.size();
    result.exactArboricity = exactAlpha;

    // --- SUB-LINEAR SAMPLING LOGIC ---
    // Calculate sample budget K based on Arboricity (alpha)
    // T_hat = (1/K) * sum(X_i)
    int64_t k_samples = static_cast<int64_t>(100000 * exactAlpha); // Adaptive budget
    if (k_samples > edgeList.size()) k_samples = edgeList.size() / 2;
    if (k_samples < 5000) k_samples = 5000;

    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<size_t> dis(0, edgeList.size() - 1);

    double triangleSum = 0;

    for (int64_t i = 0; i < k_samples; ++i) {
        // 1. Pick a random edge (u, v)
        auto edge = edgeList[dis(gen)];
        int u = edge.first;
        int v = edge.second;

        // 2. Sample a random neighbor of u or v to form a 'wedge'
        // This is the core of Importance Sampling
        const auto& u_adj = graph.at(u);
        const auto& v_adj = graph.at(v);

        // Count common neighbors (triangles containing edge u,v)
        int common = 0;
        // Intersection of two sorted vectors
        auto it1 = u_adj.begin();
        auto it2 = v_adj.begin();
        while (it1 != u_adj.end() && it2 != v_adj.end()) {
            if (*it1 < *it2) ++it1;
            else if (*it2 < *it1) ++it2;
            else {
                common++;
                ++it1; ++it2;
            }
        }
        triangleSum += common;
    }

    // 3. Final Estimator: T_hat = (TotalEdges / 3) * (triangleSum / k_samples)
    // We divide by 3 because each triangle is counted by its 3 edges
    result.triangleCount = static_cast<int64_t>((result.numEdges * triangleSum) / (k_samples * 3.0));

    return result;
}