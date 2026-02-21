#ifndef EXACT_ARBORICITY_H
#define EXACT_ARBORICITY_H

#include <vector>
#include <utility>
#include <string>
#include <functional>

struct ArboricityOutput {
    double value;          // The calculated Arboricity (gamma)
    int arboricity;       // Integer result
    int num_nodes;        // Total nodes in the graph
};

class ExactArboricity {
public:
    /**
     * Computes the exact Arboricity (gamma) using a flow-based binary search.
     * Uses C++23 views and Nash-Williams forest decomposition logic.
     */
    static ArboricityOutput compute(const std::vector<std::pair<int, int>>& edges);

private:
    struct Edge {
        int to, rev;
        double cap;
    };

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