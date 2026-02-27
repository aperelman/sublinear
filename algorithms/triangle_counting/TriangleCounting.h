#ifndef TRIANGLE_COUNTING_H
#define TRIANGLE_COUNTING_H

#include <string>
#include <vector>
#include <map>
#include <cstdint> // Required for int64_t

/**
 * Adjacency list representation.
 * Using int64_t for edge/node counts to prevent overflow on large datasets.
 */
using Graph = std::map<int, std::vector<int>>;

struct GraphAnalysisResult {
    int64_t numVertices = 0;   // n
    int64_t numEdges = 0;      // m
    int64_t triangleCount = 0; // Estimated T_hat
    double exactArboricity = 0.0; // Alpha used in the sampling budget
};

/**
 * Loads a graph from a SNAP format text file.
 */
Graph loadGraph(const std::string& filePath);

/**
 * Executes the sublinear Importance Sampling algorithm.
 * @param graph The loaded adjacency list
 * @param exactAlpha The arboricity parameter (alpha) calculated separately
 */
GraphAnalysisResult analyzeAndCount(const Graph& graph, double exactAlpha);

#endif // TRIANGLE_COUNTING_H