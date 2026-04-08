#ifndef TRIANGLE_SOLVER_H
#define TRIANGLE_SOLVER_H

#include "Snap.h"
#include <QString>

namespace TriangleSolver {
    // Exact counting
    int64_t getExactTriangleCount(const QString& filePath, int64_t& nodes, int64_t& edges);

    // Estimation logic
    int64_t estimateTriangles(const QString& filePath, double alpha_s, double eps, double T_s, int64_t m_s, int64_t& nodes, int64_t& edges);

    // Helper
    int64_t GetMaxKCore(PUNGraph Graph);
}

#endif