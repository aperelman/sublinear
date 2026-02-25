#include "triangle_algorithm.h"
#include "TriangleCounting.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

double calculateTriangleCount(const std::string& filePath, double arboricity) {
    // Load edges from file
    std::ifstream file(filePath);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + filePath);
    }
    
    std::vector<std::pair<int, int>> edges;
    std::string line;
    
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#' || line[0] == '%') {
            continue;
        }
        
        std::istringstream iss(line);
        int u, v;
        if (iss >> u >> v) {
            if (u != v) {  // Skip self-loops
                edges.emplace_back(u, v);
            }
        }
    }
    
    if (edges.empty()) {
        throw std::runtime_error("No edges found in file");
    }
    
    // Configure algorithm
    TriangleCountingConfig config{
        .arboricity = arboricity,
        .epsilon = 0.1,
        .ground_truth_triangles = std::nullopt,
        .num_trials = 10000
    };
    
    // Run algorithm
    auto result = TriangleCounting::count(edges, config);
    
    return result.estimate;
}
