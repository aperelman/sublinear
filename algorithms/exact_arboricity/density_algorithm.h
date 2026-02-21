#pragma once

#include <QString>
#include <cstdint>

// Forward declare SNAP types
class TUNGraph;
template<class T> class TPt;
typedef TPt<TUNGraph> PUNGraph;

struct DensityResult {
    bool    success          = false;
    QString errorMessage;

    int64_t numNodes         = 0;
    int64_t numEdges         = 0;
    double  edgeDensity      = 0.0;
    double  avgDegree        = 0.0;
    int     maxDegree        = 0;
    int     minDegree        = 0;
    double  degreeVariance   = 0.0;
    double  degreeStdDev     = 0.0;
    int64_t numTriangles     = 0;
    double  avgClusteringCoeff = 0.0;
    double  localDensity     = 0.0;

    QString summary() const;
};

class DensityAlgorithm {
public:
    static DensityResult compute(const PUNGraph& graph);
};
