#ifndef TRIANGLE_COUNTING_H
#define TRIANGLE_COUNTING_H

#include <string>
#include <vector>
#include <map>

namespace Triangle {

    struct GraphAnalysisResult {
        size_t numNodes;    // Changed from 'nodes' to 'numNodes'
        size_t numEdges;    // Changed from 'edges' to 'numEdges'
        size_t numTriangles; // Changed from 'triangles' to 'numTriangles'
    };

    std::map<int, std::vector<int>> loadGraph(const std::string& filename);
    GraphAnalysisResult analyzeAndCount(const std::map<int, std::vector<int>>& adj, double delta);
}

#endif