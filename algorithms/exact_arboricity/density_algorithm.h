#ifndef DENSITY_ALGORITHM_H
#define DENSITY_ALGORITHM_H

#include <map>
#include <vector>
#include <string>

namespace Arboricity {

    struct GraphAnalysisResult {
        size_t numNodes;
        size_t numEdges;
        double density;
    };

    std::map<int, std::vector<int>> loadGraph(const std::string& filename);
    GraphAnalysisResult analyzeAndCount(const std::map<int, std::vector<int>>& adj, double delta);
}

#endif