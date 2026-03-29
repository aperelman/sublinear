#include "TriangleCounting.h"
#include <algorithm>
#include <vector>
#include <thread>

namespace Triangle {

// ---------------------------------------------------------------------------
// Exact triangle counting — Chiba-Nishizeki degeneracy algorithm
//
// Complexity: O(m * alpha)  where alpha = arboricity (typically O(sqrt(m)))
// Parallelism: strided node partitioning across std::jthread workers.
//              Each thread has its own marked[] array — zero contention.
//
// Correctness:
//   Orient each edge u→v iff rank[u] < rank[v] (degeneracy order).
//   For each u, mark its out-neighbors. Then for each out-neighbor v,
//   scan ALL undirected neighbors w of v with rank[w] > rank[u].
//   If w is marked, (u,v,w) is a triangle.
//   Each triangle is counted exactly twice (for its two higher-rank vertices),
//   so divide by 2 at the end.
// ---------------------------------------------------------------------------

Result countExact(const std::vector<std::pair<int,int>>& edges)
{
    Result result{0, 0, 0};
    if (edges.empty()) return result;

    // ------------------------------------------------------------------
    // Step 1: Remap arbitrary node IDs to contiguous [0, n)
    // ------------------------------------------------------------------
    std::vector<int> allIds;
    allIds.reserve(edges.size() * 2);
    for (const auto& [u, v] : edges) {
        allIds.push_back(u);
        allIds.push_back(v);
    }
    std::sort(allIds.begin(), allIds.end());
    allIds.erase(std::unique(allIds.begin(), allIds.end()), allIds.end());

    const int n = (int)allIds.size();
    const int m = (int)edges.size();
    result.numNodes = n;
    result.numEdges = m;

    auto remap = [&](int id) -> int {
        return (int)(std::lower_bound(allIds.begin(), allIds.end(), id) - allIds.begin());
    };

    // ------------------------------------------------------------------
    // Step 2: Build undirected CSR adjacency list, deduplicated
    // ------------------------------------------------------------------
    std::vector<int> deg(n, 0);
    for (const auto& [u, v] : edges) {
        int a = remap(u), b = remap(v);
        if (a == b) continue;
        deg[a]++; deg[b]++;
    }

    std::vector<int> off(n + 1, 0);
    for (int i = 0; i < n; ++i) off[i + 1] = off[i] + deg[i];

    std::vector<int> adj(off[n]);
    {
        std::vector<int> pos(off.begin(), off.begin() + n);
        for (const auto& [u, v] : edges) {
            int a = remap(u), b = remap(v);
            if (a == b) continue;
            adj[pos[a]++] = b;
            adj[pos[b]++] = a;
        }
    }

    // Sort and deduplicate each adjacency list
    for (int i = 0; i < n; ++i) {
        int* beg = adj.data() + off[i];
        int* end = adj.data() + off[i + 1];
        std::sort(beg, end);
        int newDeg = (int)(std::unique(beg, end) - beg);
        deg[i] = newDeg;
        off[i + 1] = off[i] + newDeg;
    }
    {
        std::vector<int> adjC;
        adjC.reserve(off[n]);
        std::vector<int> offC(n + 1, 0);
        for (int i = 0; i < n; ++i) {
            offC[i + 1] = offC[i] + deg[i];
            for (int j = off[i]; j < off[i] + deg[i]; ++j)
                adjC.push_back(adj[j]);
        }
        adj = std::move(adjC);
        off = std::move(offC);
    }

    // ------------------------------------------------------------------
    // Step 3: Degeneracy ordering — O(n + m) bucket-queue peeling
    // ------------------------------------------------------------------
    const int maxDeg = *std::max_element(deg.begin(), deg.end());
    std::vector<std::vector<int>> buckets(maxDeg + 1);
    for (int i = 0; i < n; ++i) buckets[deg[i]].push_back(i);

    std::vector<int>  order;
    order.reserve(n);
    std::vector<bool> removed(n, false);
    std::vector<int>  curDeg(deg);

    for (int d = 0; (int)order.size() < n; ) {
        while (d <= maxDeg && buckets[d].empty()) ++d;
        if (d > maxDeg) break;

        int v = buckets[d].back();
        buckets[d].pop_back();

        if (removed[v])     { continue; }
        if (curDeg[v] != d) { buckets[curDeg[v]].push_back(v); continue; }

        removed[v] = true;
        order.push_back(v);

        for (int j = off[v]; j < off[v] + deg[v]; ++j) {
            int nb = adj[j];
            if (!removed[nb]) {
                int nd = --curDeg[nb];
                buckets[nd].push_back(nb);
                if (nd < d) d = nd;
            }
        }
    }

    // ------------------------------------------------------------------
    // Step 4: Degeneracy rank + oriented out-adjacency
    // ------------------------------------------------------------------
    std::vector<int> rank(n);
    for (int i = 0; i < n; ++i) rank[order[i]] = i;

    std::vector<int> outDeg(n, 0);
    for (int u = 0; u < n; ++u)
        for (int j = off[u]; j < off[u] + deg[u]; ++j)
            if (rank[adj[j]] > rank[u]) outDeg[u]++;

    std::vector<int> outOff(n + 1, 0);
    for (int i = 0; i < n; ++i) outOff[i + 1] = outOff[i] + outDeg[i];
    std::vector<int> outAdj(outOff[n]);
    {
        std::vector<int> outPos(outOff.begin(), outOff.begin() + n);
        for (int u = 0; u < n; ++u)
            for (int j = off[u]; j < off[u] + deg[u]; ++j) {
                int v = adj[j];
                if (rank[v] > rank[u]) outAdj[outPos[u]++] = v;
            }
    }
    for (int u = 0; u < n; ++u)
        std::sort(outAdj.data() + outOff[u], outAdj.data() + outOff[u + 1]);

    // ------------------------------------------------------------------
    // Step 5: Count triangles — parallel, zero-contention
    //
    // Strided partitioning spreads hub nodes evenly across threads.
    // Each thread has its own marked[] — no locks needed.
    //
    // Per node u:
    //   1. Mark all out-neighbors of u.
    //   2. For each out-neighbor v, scan undirected neighbors w of v
    //      where rank[w] > rank[u]. If marked[w]: triangle found.
    //   3. Unmark.
    //
    // Each triangle (u,v,w) with rank[u]<rank[v],rank[w] is counted twice
    // (once when processing the vertex with rank = second lowest,
    //  once when processing the vertex with rank = third lowest among
    //  the two higher-rank ones) — divide total by 2.
    // ------------------------------------------------------------------
    const int nThreads = std::max(1, (int)std::thread::hardware_concurrency());
    result.numThreads = nThreads;

    std::vector<int64_t> counts(nThreads, 0);

    auto worker = [&](int tid) {
        std::vector<uint8_t> marked(n, 0);
        int64_t local = 0;

        for (int i = tid; i < n; i += nThreads) {
            int u = order[i];
            const int u0 = outOff[u];
            const int u1 = outOff[u + 1];
            if (u0 == u1) continue;

            // Mark out-neighbors of u
            for (int j = u0; j < u1; ++j)
                marked[outAdj[j]] = 1;

            const int ru = rank[u];

            // For each out-neighbor v of u, scan undirected neighbors of v
            for (int j = u0; j < u1; ++j) {
                int v = outAdj[j];
                for (int k = off[v], kEnd = off[v] + deg[v]; k < kEnd; ++k) {
                    int w = adj[k];
                    if (rank[w] > ru)        // w ranks above u
                        local += marked[w];  // triangle if w is also out-neighbor of u
                }
            }

            // Unmark
            for (int j = u0; j < u1; ++j)
                marked[outAdj[j]] = 0;
        }

        counts[tid] = local;
    };

    {
        std::vector<std::jthread> threads;
        threads.reserve(nThreads);
        for (int t = 0; t < nThreads; ++t)
            threads.emplace_back(worker, t);
    } // jthreads join on scope exit

    int64_t total = 0;
    for (int64_t c : counts) total += c;
    result.numTriangles = total / 2;

    return result;
}

} // namespace Triangle
