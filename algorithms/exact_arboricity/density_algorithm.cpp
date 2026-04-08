#include "density_algorithm.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace Arboricity {

    // Implementation of graph loading for Arboricity
    std::map<int, std::vector<int>> loadGraph(const std::string& filename) {
        std::map<int, std::vector<int>> adj;
        std::ifstream infile(filename);
        if (!infile.is_open()) return adj;

        std::string line;
        while (std::getline(infile, line)) {
            // Skip SNAP/MTX comments
            if (line.empty() || line[0] == '#' || line[0] == '%') continue;

            std::stringstream ss(line);
            int u, v;
            if (ss >> u >> v) {
                if (u == v) continue; // Ignore self-loops
                adj[u].push_back(v);
                adj[v].push_back(u);
            }
        }

        // Standardize adjacency lists
        for (auto& pair : adj) {
            std::sort(pair.second.begin(), pair.second.end());
            pair.second.erase(std::unique(pair.second.begin(), pair.second.end()), pair.second.end());
        }

        return adj;
    }

    // Implementation of density analysis
    GraphAnalysisResult analyzeAndCount(const std::map<int, std::vector<int>>& adj, double delta) {
        GraphAnalysisResult result = {0, 0, 0.0};

        if (adj.empty()) return result;

        result.numNodes = adj.size();

        long long total_degree = 0;
        for (const auto& [u, neighbors] : adj) {
            total_degree += neighbors.size();
        }

        // Undirected edge count
        result.numEdges = total_degree / 2;

        // Density formula: |E| / |V|
        if (result.numNodes > 0) {
            result.density = static_cast<double>(result.numEdges) / static_cast<double>(result.numNodes);
        }

        return result;
    }
}