#ifndef TRIANGLE_COUNTING_H
#define TRIANGLE_COUNTING_H

#include <vector>
#include <utility>
#include <cstdint>

namespace Triangle {

struct Result {
    int64_t numNodes;
    int64_t numEdges;
    int64_t numTriangles;
    int     numThreads = 1;  // actual OpenMP threads used
};

/**
 * Exact triangle counting using the Chiba-Nishizeki degeneracy-ordered algorithm.
 * Parallelized with OpenMP. Uses hybrid marking/intersection/binary-search strategy.
 *
 * Reference: Chiba & Nishizeki, "Arboricity and Subgraph Listing Algorithms",
 *            SIAM J. Comput. 14(1), 1985.
 *
 * @param edges  Edge list as pairs of node IDs (arbitrary, non-contiguous IDs ok)
 * @return       Result struct with node/edge/triangle counts and thread count
 */
Result countExact(
    const std::vector<std::pair<int,int>>& edges
);

} // namespace Triangle

#endif // TRIANGLE_COUNTING_H
