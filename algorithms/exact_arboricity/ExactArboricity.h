#ifndef EXACT_ARBORICITY_H
#define EXACT_ARBORICITY_H

#include <vector>
#include <utility>
#include <string>
#include <functional>

struct ArboricityOutput {
    double value;
    int arboricity;
    int num_nodes;
};

class ExactArboricity {
public:
    using LogFn = std::function<void(const std::string&)>;

    static ArboricityOutput compute(
        const std::vector<std::pair<int, int>>& edges,
        int degeneracy = -1,
        LogFn log = nullptr);

private:
    struct Edge {
        int to;
        double cap;
        size_t rev;
    };

    class MaxFlowSolver {
        int n;
        std::vector<std::vector<Edge>> adj;
        std::vector<int> height, count;
        std::vector<double> excess;

    public:
        explicit MaxFlowSolver(int n);
        void addEdge(int u, int v, double cap);
        double solve(int s, int t);
    };
};

#endif