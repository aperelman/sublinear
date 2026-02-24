#ifndef EXACT_ARBORICITY_H
#define EXACT_ARBORICITY_H

#include <vector>
#include <utility>
#include <string>
#include <functional>

struct ArboricityOutput {
    double value;      // The calculated arboricity (gamma)
    int arboricity;    // Integer result
    int num_nodes;     // Total nodes in the graph
};

class ExactArboricity {
public:
    using LogFn = std::function<void(const std::string&)>;

    static ArboricityOutput compute(
        const std::vector<std::pair<int, int>>& edges,
        LogFn log = nullptr);

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
