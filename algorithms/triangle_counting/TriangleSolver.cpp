#include "TriangleSolver.h"
#include <random>
#include <vector>
#include <algorithm>
int64_t TriangleSolver::GetMaxKCore(PUNGraph Graph) {
    TIntPrV CoreIdSzV;
    TSnap::GetKCoreNodes(Graph, CoreIdSzV);
    if (CoreIdSzV.Empty()) return 1;
    // CoreIdSzV is sorted by k; last entry has highest k
    return static_cast<int64_t>(CoreIdSzV.Last().Val1.Val);
}

int64_t TriangleSolver::getExactTriangleCount(const QString& filePath, int64_t& nodes, int64_t& edges) {
    // SNAP uses C-style strings
    PNGraph Graph = TSnap::LoadEdgeList<PNGraph>(filePath.toStdString().c_str(), 0, 1);

    nodes = static_cast<int64_t>(Graph->GetNodes());
    edges = static_cast<int64_t>(Graph->GetEdges());

    return static_cast<int64_t>(TSnap::GetTriads(Graph));
}

int64_t TriangleSolver::estimateTriangles(const QString& filePath, double alpha_s, double eps,
                                          double T_s, int64_t m_s, int64_t& nodes, int64_t& edges) {

    PUNGraph Graph = TSnap::LoadEdgeList<PUNGraph>(filePath.toStdString().c_str(), 0, 1);
    nodes = static_cast<int64_t>(Graph->GetNodes());

    // Use user-provided edge count or get from graph
    int64_t m = (m_s > 0) ? m_s : static_cast<int64_t>(Graph->GetEdges());
    edges = m;

    // Parameter setup
    double alpha_ref = (alpha_s <= 0) ? static_cast<double>(GetMaxKCore(Graph)) : alpha_s;
    double T_ref = (T_s > 0) ? T_s : static_cast<double>(TSnap::GetTriads(Graph));

    double t_prime = std::max(1.0, 0.1 * T_ref);
//    int64_t r = std::max(1000L, static_cast<int64_t>((m * alpha_ref) / (eps * t_prime)));
    res = std::max(static_cast<int64_t>(a), static_cast<int64_t>(b));
    double p1 = std::min(1.0, static_cast<double>(r) / static_cast<double>(m));

    // Sampling
    std::vector<TIntPr> R;
    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    for (auto EI = Graph->BegEI(); EI < Graph->EndEI(); EI++) {
        if (dis(gen) < p1) {
            R.push_back(TIntPr(EI.GetSrcNId(), EI.GetDstNId()));
        }
    }

    if (R.empty()) return 0;

    // Estimation loop
    int64_t X = 0;
    for (int64_t i = 0; i < r; ++i) {
        TIntPr e = R[i % R.size()];

        // Min-degree vertex selection
        int u = (Graph->GetNI(e.Val1.Val).GetDeg() < Graph->GetNI(e.Val2.Val).GetDeg())
                ? e.Val1.Val : e.Val2.Val;
        int v = (u == e.Val1.Val) ? e.Val2.Val : e.Val1.Val;

        auto NI_u = Graph->GetNI(u);
        if (NI_u.GetDeg() > 0) {
            int w = NI_u.GetNbrNId(std::uniform_int_distribution<>(0, NI_u.GetDeg() - 1)(gen));
            if (Graph->IsEdge(v, w)) X++;
        }
    }

    return static_cast<int64_t>((static_cast<double>(X) * m) / static_cast<double>(r));
}

