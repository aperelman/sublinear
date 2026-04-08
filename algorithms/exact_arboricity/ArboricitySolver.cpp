#include "ArboricitySolver.h"

#include <algorithm>
#include <unordered_map>
#include <cassert>
#include <cmath>
#include <limits>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <thread>
#include <atomic>
#include <mutex>
#include <thread>
#include <atomic>
#include <mutex>

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
static constexpr long INF_CAP = std::numeric_limits<long>::max() / 2;

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
ArboricitySolver::ArboricitySolver(int numNodes)
    : m_numNodes(numNodes)
{
    if (numNodes < 0)
        throw std::invalid_argument("numNodes must be >= 0");
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------
void ArboricitySolver::addEdge(int u, int v)
{
    m_edges.push_back({u, v});
}

const std::vector<int>& ArboricitySolver::getDensestSubgraph() const
{
    return m_densestSubgraph;
}

// ---------------------------------------------------------------------------
// Main entry point
// ---------------------------------------------------------------------------
int ArboricitySolver::computeExact(ProgressFn onProgress, LogFn log)
{
    auto lg = [&](const std::string& msg) { if (log) log(msg); };

    if (m_edges.empty() || m_numNodes <= 1) return 0;

    m_nThreads = std::max(1, (int)std::thread::hardware_concurrency());

    // Remap node IDs to contiguous [0, N) FIRST — before computeDegeneracy
    // or any other operation that uses node IDs as array indices.
    // SNAP graphs have non-contiguous IDs (e.g. com-Amazon has IDs up to 548K
    // but only 334K distinct nodes). Without remapping, array indices overflow.
    {
        std::unordered_map<int,int> idMap;
        idMap.reserve(m_edges.size() * 2);
        int nextId = 0;
        for (auto& e : m_edges) {
            if (idMap.find(e.first)  == idMap.end()) idMap[e.first]  = nextId++;
            if (idMap.find(e.second) == idMap.end()) idMap[e.second] = nextId++;
        }
        for (auto& e : m_edges) {
            e.first  = idMap[e.first];
            e.second = idMap[e.second];
        }
        m_numNodes = nextId;
    }

    const int M = static_cast<int>(m_edges.size());
    const int N = m_numNodes;

    if (N <= 1) return 0;

    // --- Bounds ---------------------------------------------------------
    const int degeneracy = computeDegeneracy();
    int lo = static_cast<int>(
                 std::ceil(static_cast<double>(M) / (N - 1)));
    int hi = computeUpperBound(degeneracy);

    lo = std::max(lo, 1);
    hi = std::max(hi, lo);

    lg("Arboricity bounds: lo=" + std::to_string(lo) +
       " hi=" + std::to_string(hi) +
       " (degeneracy=" + std::to_string(degeneracy) + ")");

    // --- Fast path: lo is often the answer in sparse real graphs --------
    buildNetwork(lo);
    if (runMaxFlow() == M) {
        lg("Fast path: arboricity = " + std::to_string(lo));
        extractDensestSubgraph();
        return lo;
    }

    // lo is infeasible — move to binary search starting just above lo
    int best = hi;
    int searchLo = lo + 1;

    // Build at searchLo so we enter the loop with a valid network
    // (we already know lo is invalid, so start fresh at searchLo)
    buildNetwork(searchLo);
    long currentFlow = runMaxFlow();
    if (currentFlow == M) {
        best = searchLo;
        hi   = searchLo - 1; // will exit loop immediately
    }
    m_currentK = searchLo;

    // --- Binary search with warm-start ----------------------------------
    int searchHi = hi;
    while (searchLo <= searchHi) {
        int mid = searchLo + (searchHi - searchLo) / 2;

        if (onProgress) onProgress(mid, searchLo, searchHi);
        lg("Testing k=" + std::to_string(mid) +
           " [" + std::to_string(searchLo) + ".." + std::to_string(searchHi) + "]");

        // Warm-start: just patch sink capacities
        if (mid != m_currentK) {
            if (mid > m_currentK) {
                // Increasing k: old flow still feasible, add capacity
                patchSinkCapacity(m_currentK, mid);
                // No need to reset — partial flow is still valid
            } else {
                // Decreasing k: old flow may violate new sink caps, reset
                patchSinkCapacity(m_currentK, mid);
                resetFlow();
            }
            m_currentK = mid;
        }

        long flow = runMaxFlow();
        bool feasible = (flow == M);

        lg(std::string(" -> ") + (feasible ? "feasible" : "infeasible"));

        if (feasible) {
            best     = mid;
            searchHi = mid - 1;
            // Flow stays warm for next (lower) k — but we'll reset there
        } else {
            searchLo = mid + 1;
            // Flow stays warm for next (higher) k — can reuse
        }
    }

    lg("Arboricity = " + std::to_string(best));

    // Final run at best_k to populate densest subgraph
    buildNetwork(best);
    runMaxFlow();
    extractDensestSubgraph();

    return best;
}

// ---------------------------------------------------------------------------
// Flow network construction
// ---------------------------------------------------------------------------
//
// Node layout:
//   source       = 0
//   edge-node i  = 1 + i              (i in 0..M-1)
//   vertex-node v = 1 + M + v          (v in 0..N-1)
//   sink         = 1 + M + N
//
void ArboricitySolver::buildNetwork(int k)
{
    const int M = static_cast<int>(m_edges.size());
    const int N = m_numNodes;

    m_source = 0;
    m_sink   = 1 + M + N;
    m_flowN  = m_sink + 1;

    m_graph.assign(m_flowN, {});

    // Source → edge-nodes (capacity 1 each)
    for (int i = 0; i < M; ++i) {
        addFlowEdge(m_source, 1 + i, 1L);
    }

    // Edge-nodes → their two endpoint vertex-nodes (capacity INF)
    for (int i = 0; i < M; ++i) {
        int eu = 1 + i;
        int vu = 1 + M + m_edges[i].first;
        int vv = 1 + M + m_edges[i].second;
        addFlowEdge(eu, vu, INF_CAP);
        addFlowEdge(eu, vv, INF_CAP);
    }

    // Vertex-nodes → sink (capacity k each)
    for (int v = 0; v < N; ++v) {
        addFlowEdge(1 + M + v, m_sink, static_cast<long>(k));
    }

    m_currentK = k;
    m_level.assign(m_flowN, -1);
    m_ptr.assign(m_flowN, 0);
}

// ---------------------------------------------------------------------------
// Warm-start: patch sink capacities from oldK to newK (O(N))
// ---------------------------------------------------------------------------
void ArboricitySolver::patchSinkCapacity(int oldK, int newK)
{
    const int M   = static_cast<int>(m_edges.size());
    const long delta = static_cast<long>(newK - oldK);

    for (int v = 0; v < m_numNodes; ++v) {
        int vnode = 1 + M + v;
        // The sink edge is the last edge added from vnode → find it
        // (it's always the edge to m_sink added in buildNetwork)
        for (auto& e : m_graph[vnode]) {
            if (e.to == m_sink) {
                e.cap += delta;
                // Update reverse edge cap on sink side
                m_graph[m_sink][e.rev].cap -= delta;
                break;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Reset all flow values to 0 by rebuilding the network at currentK.
// O(V+E) — only called when k decreases (binary search goes feasible→lower),
// which happens at most O(log(hi-lo)) times total.
// ---------------------------------------------------------------------------
void ArboricitySolver::resetFlow()
{
    buildNetwork(m_currentK);
}

// ---------------------------------------------------------------------------
// Parallel BFS — level graph construction using atomic level array
// Each frontier level is processed in parallel by m_nThreads threads.
// ---------------------------------------------------------------------------
bool ArboricitySolver::bfs()
{
    // Use atomic ints for thread-safe level assignment
    std::vector<std::atomic<int>> level(m_flowN);
    for (auto& l : level) l.store(-1, std::memory_order_relaxed);
    level[m_source].store(0, std::memory_order_relaxed);

    // Current frontier and next frontier
    std::vector<int> frontier;
    frontier.reserve(m_flowN);
    frontier.push_back(m_source);

    bool reachedSink = false;

    while (!frontier.empty() && !reachedSink) {
        std::vector<int> nextFrontier;
        std::mutex nextMtx;

        // Partition frontier across threads
        const int fSize = (int)frontier.size();
        const int chunk = std::max(1, fSize / m_nThreads);

        std::vector<std::thread> threads;
        threads.reserve(m_nThreads);
        std::atomic<bool> sinkFound{false};

        for (int t = 0; t < m_nThreads; ++t) {
            int lo = t * chunk;
            int hi = (t == m_nThreads - 1) ? fSize : std::min(lo + chunk, fSize);
            if (lo >= fSize) break;

            threads.emplace_back([&, lo, hi]() {
                std::vector<int> localNext;
                for (int fi = lo; fi < hi; ++fi) {
                    int u = frontier[fi];
                    int uLevel = level[u].load(std::memory_order_relaxed);
                    for (const auto& e : m_graph[u]) {
                        if (e.cap <= 0) continue;
                        int expected = -1;
                        if (level[e.to].compare_exchange_strong(
                                expected, uLevel + 1,
                                std::memory_order_relaxed)) {
                            if (e.to == m_sink)
                                sinkFound.store(true, std::memory_order_relaxed);
                            localNext.push_back(e.to);
                        }
                    }
                }
                if (!localNext.empty()) {
                    std::lock_guard<std::mutex> lk(nextMtx);
                    nextFrontier.insert(nextFrontier.end(),
                                        localNext.begin(), localNext.end());
                }
            });
        }
        for (auto& th : threads) th.join();

        if (sinkFound) { reachedSink = true; }
        frontier = std::move(nextFrontier);
    }

    // Copy atomic levels back to m_level for use in augmentation
    m_level.resize(m_flowN);
    for (int i = 0; i < m_flowN; ++i)
        m_level[i] = level[i].load(std::memory_order_relaxed);

    return reachedSink;
}

// ---------------------------------------------------------------------------
// Dinic's iterative DFS + BFS — see runMaxFlow below
// ---------------------------------------------------------------------------

long ArboricitySolver::runMaxFlow()
{
    const int nThreads = std::max(1, (int)std::thread::hardware_concurrency());
    const int M = (int)m_edges.size();

    // Shared atomic total flow
    std::atomic<long> totalFlow{0};

    // Each BFS phase is sequential (builds level graph).
    // Augmentation phase runs in parallel: each thread handles
    // a strided subset of source→edge-node edges.
    // Since source edges have capacity 1 and are disjoint per thread,
    // the only contention is on vertex→sink edges (capacity k).
    // We use per-edge mutexes for those edges only.

    // Build mutex per vertex-node → sink edge (N mutexes)
    const int N = m_numNodes;
    std::vector<std::mutex> sinkMtx(N);

    while (bfs()) {
        // Reset advance pointers
        std::fill(m_ptr.begin(), m_ptr.end(), 0);

        std::atomic<bool> anyAugmented{false};

        // Launch threads — each handles source edges [tid, tid+nThreads, ...]
        auto worker = [&](int tid) {
            // Each thread has its own path stack
            struct Frame { int node; long flow; int edge; };
            std::vector<Frame> stk;
            stk.reserve(64);

            // Iterate over source edges assigned to this thread
            auto& srcEdges = m_graph[m_source];
            for (int si = tid; si < (int)srcEdges.size(); si += nThreads) {
                auto& se = srcEdges[si];
                if (se.cap <= 0) continue;
                if (m_level[se.to] != 1) continue;

                // Try to find an augmenting path starting from this edge-node
                stk.clear();
                stk.push_back({m_source, 1L, si});

                while (!stk.empty()) {
                    Frame& cur = stk.back();
                    int u = cur.node;

                    if (u == m_sink) {
                        // Augment path
                        long f = cur.flow;

                        // Check and acquire sink mutex for vertex-node
                        // Find the vertex-node just before sink
                        int vnode = stk[stk.size()-2].node;
                        int vnIdx = vnode - 1 - M; // vertex index

                        bool acquired = false;
                        if (vnIdx >= 0 && vnIdx < N) {
                            acquired = sinkMtx[vnIdx].try_lock();
                            if (!acquired) {
                                // Another thread is using this sink edge — backtrack
                                m_level[u] = -1;
                                stk.pop_back();
                                continue;
                            }
                        }

                        // Augment
                        totalFlow.fetch_add(f, std::memory_order_relaxed);
                        anyAugmented.store(true, std::memory_order_relaxed);

                        for (int i = (int)stk.size()-1; i > 0; --i) {
                            int parent = stk[i-1].node;
                            int ei     = stk[i].edge;
                            auto& fwd  = m_graph[parent][ei];
                            auto& rev  = m_graph[fwd.to][fwd.rev];
                            fwd.cap -= f;
                            rev.cap += f;
                        }

                        if (acquired && vnIdx >= 0 && vnIdx < N)
                            sinkMtx[vnIdx].unlock();

                        stk.pop_back();
                        break; // one path per source edge (cap=1)
                    }

                    // Advance pointer
                    bool pushed = false;
                    auto& edges = m_graph[u];
                    for (int& i = m_ptr[u]; i < (int)edges.size(); ++i) {
                        const auto& e = edges[i];
                        if (e.cap <= 0) continue;
                        if (m_level[e.to] != m_level[u] + 1) continue;
                        stk.push_back({e.to, std::min(cur.flow, e.cap), i});
                        pushed = true;
                        break;
                    }

                    if (!pushed) {
                        m_level[u] = -1;
                        stk.pop_back();
                    }
                }
            }
        };

        // Run workers
        if (nThreads == 1) {
            worker(0);
        } else {
            std::vector<std::thread> threads;
            threads.reserve(nThreads);
            for (int t = 0; t < nThreads; ++t)
                threads.emplace_back(worker, t);
            for (auto& th : threads) th.join();
        }

        if (!anyAugmented) break;
    }

    return totalFlow.load();
}

// ---------------------------------------------------------------------------
// Add a directed edge with its reverse (residual)
// ---------------------------------------------------------------------------
void ArboricitySolver::addFlowEdge(int u, int v, long cap)
{
    m_graph[u].push_back({v, cap, static_cast<int>(m_graph[v].size())});
    m_graph[v].push_back({u, 0L, static_cast<int>(m_graph[u].size()) - 1});
}

// ---------------------------------------------------------------------------
// Degeneracy via repeated minimum-degree peeling (O(V + E))
// ---------------------------------------------------------------------------
int ArboricitySolver::computeDegeneracy() const
{
    const int N = m_numNodes;
    std::vector<int> deg(N, 0);
    std::vector<std::vector<int>> adj(N);

    for (const auto& [u, v] : m_edges) {
        adj[u].push_back(v);
        adj[v].push_back(u);
        deg[u]++;
        deg[v]++;
    }

    // O(V+E) bucket queue with O(1) removal via position tracking
    int maxDeg = *std::max_element(deg.begin(), deg.end());
    std::vector<std::vector<int>> buckets(maxDeg + 1);
    std::vector<int> pos(N);  // pos[v] = index of v in its bucket

    for (int i = 0; i < N; ++i) {
        pos[i] = (int)buckets[deg[i]].size();
        buckets[deg[i]].push_back(i);
    }

    std::vector<bool> removed(N, false);
    int degeneracy = 0;
    int d = 0;

    for (int processed = 0; processed < N; ++processed) {
        // Advance to next non-empty bucket
        while (d <= maxDeg && buckets[d].empty()) ++d;

        // Pop last element from bucket d (O(1))
        int u = buckets[d].back();
        buckets[d].pop_back();
        removed[u] = true;
        degeneracy = std::max(degeneracy, deg[u]);

        for (int w : adj[u]) {
            if (removed[w]) continue;

            // O(1) removal of w from its current bucket
            int bd = deg[w];
            int p  = pos[w];
            int last = buckets[bd].back();
            buckets[bd][p] = last;
            pos[last] = p;
            buckets[bd].pop_back();

            // Move w to bucket deg[w]-1
            deg[w]--;
            pos[w] = (int)buckets[deg[w]].size();
            buckets[deg[w]].push_back(w);
            if (deg[w] < d) d = deg[w];
        }
    }

    return degeneracy;
}

// ---------------------------------------------------------------------------
// Upper bound: degeneracy is always a valid upper bound for arboricity.
// We also compute ceil(m/(n-1)) as the lower bound (done in computeExact).
// An additional tighter upper bound: ceil(maxDegree / 2) is valid for
// simple graphs but can fail for multigraphs — so we take the min safely.
// ---------------------------------------------------------------------------
int ArboricitySolver::computeUpperBound(int degeneracy) const
{
    if (m_edges.empty()) return 0;

    std::vector<int> deg(m_numNodes, 0);
    for (const auto& [u, v] : m_edges) { deg[u]++; deg[v]++; }
    int maxDeg = *std::max_element(deg.begin(), deg.end());

    // ceil(maxDeg / 2) is a valid upper bound: each edge contributes
    // at most 1 to the degree of each endpoint, so the densest vertex
    // needs at most ceil(deg/2) forests to cover its incident edges.
    int bound2 = (maxDeg + 1) / 2;

    // Take min with degeneracy — both are valid upper bounds
    return std::max(1, std::min(degeneracy, bound2));
}

// ---------------------------------------------------------------------------
// Extract densest subgraph from the last min-cut (call after runMaxFlow)
// ---------------------------------------------------------------------------
void ArboricitySolver::extractDensestSubgraph()
{
    // Nodes reachable from source in the residual graph belong to the
    // source side of the min-cut — these are the densest subgraph vertices.
    const int M = static_cast<int>(m_edges.size());

    // BFS on residual
    std::vector<bool> reachable(m_flowN, false);
    std::queue<int> q;
    q.push(m_source);
    reachable[m_source] = true;
    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (const auto& e : m_graph[u]) {
            if (e.cap > 0 && !reachable[e.to]) {
                reachable[e.to] = true;
                q.push(e.to);
            }
        }
    }

    m_densestSubgraph.clear();
    for (int v = 0; v < m_numNodes; ++v) {
        if (reachable[1 + M + v])
            m_densestSubgraph.push_back(v);
    }
}
