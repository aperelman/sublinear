#include "TriangleCounting.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <map>

namespace Triangle {

    // Implementation of graph loading
    std::map<int, std::vector<int>> loadGraph(const std::string& filename) {
        std::map<int, std::vector<int>> adj;
        std::ifstream infile(filename);
        if (!infile.is_open()) return adj;

        std::string line;
        while (std::getline(infile, line)) {
            // Skip comments and empty lines
            if (line.empty() || line[0] == '#' || line[0] == '%') continue;

            std::stringstream ss(line);
            int u, v;
            if (ss >> u >> v) {
                if (u == v) continue; // Skip self-loops
                adj[u].push_back(v);
                adj[v].push_back(u);
            }
        }

        // Clean up adjacency list: sort and remove duplicates for binary search efficiency
        for (auto& pair : adj) {
            std::sort(pair.second.begin(), pair.second.end());
            pair.second.erase(std::unique(pair.second.begin(), pair.second.end()), pair.second.end());
        }

        return adj;
    }

    // Implementation of triangle counting
    GraphAnalysisResult analyzeAndCount(const std::map<int, std::vector<int>>& adj, double delta) {
        GraphAnalysisResult result = {0, 0, 0};
        result.numNodes = adj.size();

        long long total_degree = 0;
        for (const auto& [u, neighbors] : adj) {
            total_degree += neighbors.size();
        }
        result.numEdges = total_degree / 2;

        long long triangle_count = 0;

        // Iterate through each vertex u
        for (const auto& [u, neighbors] : adj) {
            // Check every pair of neighbors (v, w)
            for (size_t i = 0; i < neighbors.size(); ++i) {
                for (size_t j = i + 1; j < neighbors.size(); ++j) {
                    int v = neighbors[i];
                    int w = neighbors[j];

                    // To avoid triple-counting, we only check if u < v < w
                    if (u < v && v < w) {
                        const auto& v_neighbors = adj.at(v);
                        // Binary search for w in v's adjacency list
                        if (std::binary_search(v_neighbors.begin(), v_neighbors.end(), w)) {
                            triangle_count++;
                        }
                    }
                }
            }
        }

        result.numTriangles = triangle_count;
        return result;
    }
}