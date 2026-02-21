#ifndef EXACT_ARBORICITY_H
#define EXACT_ARBORICITY_H

#include <vector>
#include <utility>
#include <unordered_map>

struct ArboricityOutput {
    double max_avg_degree; // The exact alpha_0 (before ceiling)
    int alpha_0;           // ceil(max_avg_degree)
    int num_nodes_in_dense_subgraph;
};

class ExactArboricity {
public:
    /**
     * Computes the exact Maximum Average Degree (alpha_0) using Goldberg's Algorithm.
     * Scales to large graphs using Push-Relabel with Gap Heuristic.
     */
    static ArboricityOutput compute(const std::vector<std::pair<int, int>>& edges);

private:
    struct Edge {
        int to, rev;
        double cap;
    };

    // Internal Max-Flow implementation (Push-Relabel)
    class MaxFlowSolver {
        int n;
        std::vector<std::vector<Edge>> adj;
        std::vector<int> height, count;
        std::vector<double> excess;

    public:
        MaxFlowSolver(int n);
        void addEdge(int u, int v, double cap);
        double solve(int s, int t);
    };
};

#endif // EXACT_ARBORICITY_H