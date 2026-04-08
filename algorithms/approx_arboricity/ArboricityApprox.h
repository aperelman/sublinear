#ifndef ARBORICITY_APPROX_H
#define ARBORICITY_APPROX_H

#include <vector>
#include <utility>
#include <functional>
#include <string>

/**
 * @brief Fast approximate arboricity via greedy edge orientation (peeling).
 *
 * Algorithm: repeatedly remove the minimum-degree node (degeneracy ordering).
 * The maximum out-degree in the resulting orientation is an upper bound on
 * arboricity and equals the degeneracy. By Nash-Williams:
 *   ceil(degeneracy / 2) <= arboricity <= degeneracy
 *
 * For most real-world graphs arboricity ≈ degeneracy/2 to degeneracy.
 * This runs in O(V+E) using a bucket queue — same as computeDegeneracy()
 * in ArboricitySolver but returns ceil(M_S / (|S|-1)) over all subgraphs S.
 *
 * The greedy peeling bound is:
 *   max over all subsets S: ceil(|E(S)| / (|S| - 1))
 * which equals exact arboricity by Nash-Williams theorem.
 * We approximate it by tracking the densest subgraph during peeling.
 */
class ArboricityApprox {
public:
    using LogFn = std::function<void(const std::string&)>;

    static int compute(const std::vector<std::pair<int,int>>& edges,
                       int numNodes,
                       LogFn log = nullptr);
};

#endif // ARBORICITY_APPROX_H
