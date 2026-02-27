#ifndef EXACT_ARBORICITY_H
#define EXACT_ARBORICITY_H

#include <vector>
#include <string>
#include <functional>
#include <utility>

/**
 * Result structure returned by the compute function.
 */
struct ArboricityOutput {
    double alpha;
    int k;
    int nodes;
};

class ExactArboricity {
public:
    // Required by: ExactArboricity::compute(..., LogFn log)
    using LogFn = std::function<void(const std::string&)>;

    /**
     * Calculates exact arboricity using Nash-Williams forest cover logic.
     * Matches the implementation in your ExactArboricity.cpp.
     */
    static ArboricityOutput compute(
        const std::vector<std::pair<int, int>>& edges,
        LogFn log = nullptr
    );

private:
    // Internal edge structure for the MaxFlowSolver
    struct Edge {
        int to;
        int rev;
        double cap;
    };

    /**
     * Inner class implementing the Push-Relabel (Pre-flow push) algorithm.
     * Matches: ExactArboricity::MaxFlowSolver::MaxFlowSolver(int n)
     */
    class MaxFlowSolver {
    public:
        MaxFlowSolver(int n);
        void addEdge(int u, int v, double cap);
        double solve(int s, int t);

    private:
        int n;
        std::vector<std::vector<Edge>> adj;
        std::vector<int> height;
        std::vector<int> count;
        std::vector<double> excess;
    };
};

#endif // EXACT_ARBORICITY_H