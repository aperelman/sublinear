#!/usr/bin/env python3
"""
Large-Set-Arboricity Algorithms - OPTIMIZED
Based on: "My notes for prove approximate for large-Set-Arboricity" by Amit Perelman

Key optimizations:
1. Heap-based minimum degree selection: O(n) → O(n log n)
2. Incremental degree updates instead of recomputation
3. Adjacency list caching for fast neighbor access
"""

import networkx as nx
import math
from typing import Tuple, List, Optional
from itertools import combinations
import heapq
import time


class LargeSetArboricityOptimized:
    """
    Optimized implementation of large-set-arboricity algorithms.
    
    The large-set-arboricity αk(G) is defined as:
        αk(G) = max_{G' ⊆ G, |V(G')| > k} ⌈d̄[G']⌉
    
    where d̄[G'] is the average degree of subgraph G'.
    """
    
    def __init__(self, G: nx.Graph):
        """Initialize with a NetworkX graph"""
        self.G = G.copy()
        self.n = G.number_of_nodes()
        # Cache adjacency for faster access
        self.adj = {v: set(G.neighbors(v)) for v in G.nodes()}
    
    def modified_degeneracy_algorithm_optimized(self, k: int) -> Tuple[int, List[int]]:
        """
        OPTIMIZED Modified Degeneracy Algorithm using min-heap
        
        Complexity: O(m log n) instead of O(n²m)
        
        Args:
            k: Parameter (size of large set)
        
        Returns:
            (dk_value, removal_order)
        """
        n = self.n
        if k > n:
            k = n
        if k <= 0:
            return 0, []
        
        # Build adjacency list (mutable copy)
        neighbors = {v: set(self.adj[v]) for v in self.G.nodes()}
        
        # Initialize heap with (degree, vertex) pairs
        degrees = {v: len(neighbors[v]) for v in self.G.nodes()}
        heap = [(degrees[v], v) for v in self.G.nodes()]
        heapq.heapify(heap)
        
        removed = set()
        removal_order = []
        degree_at_removal = {}
        
        # Remove vertices one by one
        while heap:
            # Get minimum degree vertex
            deg, v = heapq.heappop(heap)
            
            # Skip if already removed (lazy deletion)
            if v in removed:
                continue
            
            # Current actual degree (may have changed)
            actual_deg = degrees[v]
            if deg < actual_deg:
                # Degree increased, re-insert with correct degree
                heapq.heappush(heap, (actual_deg, v))
                continue
            
            # Remove vertex
            removed.add(v)
            removal_order.append(v)
            degree_at_removal[v] = actual_deg
            
            # Update neighbors' degrees
            for u in neighbors[v]:
                if u not in removed:
                    neighbors[u].discard(v)
                    degrees[u] -= 1
                    # Push updated degree (lazy deletion handles duplicates)
                    heapq.heappush(heap, (degrees[u], u))
            
            # Clear v's neighbors
            neighbors[v].clear()
        
        # dk(G) = max degree in last k vertices
        last_k = removal_order[-k:] if k <= len(removal_order) else removal_order
        dk_value = max(degree_at_removal[v] for v in last_k) if last_k else 0
        
        return dk_value, removal_order
    
    def modified_degeneracy_algorithm(self, k: int) -> Tuple[int, List[int]]:
        """Original algorithm for comparison"""
        n = self.n
        if k > n:
            k = n
        if k <= 0:
            return 0, []
        
        H = self.G.copy()
        removal_order = []
        degree_at_removal = {}
        
        # Remove vertices one by one (minimum degree first)
        for step in range(n):
            if H.number_of_nodes() == 0:
                break
            
            # Find minimum degree vertex - O(n) scan
            min_deg = float('inf')
            min_vertex = None
            for v in H.nodes():
                deg = H.degree(v)
                if deg < min_deg:
                    min_deg = deg
                    min_vertex = v
            
            # Record and remove
            removal_order.append(min_vertex)
            degree_at_removal[min_vertex] = min_deg
            H.remove_node(min_vertex)
        
        # dk(G) = max degree in last k vertices
        last_k = removal_order[-k:] if k <= len(removal_order) else removal_order
        dk_value = max(degree_at_removal[v] for v in last_k) if last_k else 0
        
        return dk_value, removal_order
    
    def compute_alpha_k_exact(self, k: int) -> Tuple[int, Optional[nx.Graph]]:
        """
        Compute exact αk(G) by checking all subgraphs with |V| > k
        WARNING: Exponential time! Only works for small graphs (n ≤ 15)
        """
        n = self.n
        if n <= k:
            return 0, None
        
        if n > 15:
            print(f"Warning: Graph too large (n={n}) for exact αk computation")
            return None, None
        
        nodes = list(self.G.nodes())
        max_alpha = 0
        best_subgraph = None
        
        # Check all subsets of size > k
        for size in range(k + 1, n + 1):
            for subset in combinations(nodes, size):
                subgraph = self.G.subgraph(subset)
                if subgraph.number_of_nodes() > 0 and subgraph.number_of_edges() > 0:
                    avg_deg = 2 * subgraph.number_of_edges() / subgraph.number_of_nodes()
                    alpha_val = math.ceil(avg_deg)
                    if alpha_val > max_alpha:
                        max_alpha = alpha_val
                        best_subgraph = subgraph.copy()
        
        return max_alpha, best_subgraph
    
    def verify_approximation(self, k: int, use_optimized: bool = True) -> dict:
        """
        Verify that dk(G) ≤ αk(G) ≤ 2·dk(G)
        """
        if use_optimized:
            dk_value, _ = self.modified_degeneracy_algorithm_optimized(k)
        else:
            dk_value, _ = self.modified_degeneracy_algorithm(k)
            
        alpha_k, _ = self.compute_alpha_k_exact(k)
        
        if alpha_k is None:
            return {
                'k': k,
                'dk': dk_value,
                'alpha_k': None,
                'lower_bound_ok': None,
                'upper_bound_ok': None,
                'ratio': None
            }
        
        lower_ok = (dk_value <= alpha_k)
        upper_ok = (alpha_k <= 2 * dk_value)
        ratio = alpha_k / dk_value if dk_value > 0 else float('inf')
        
        return {
            'k': k,
            'dk': dk_value,
            'alpha_k': alpha_k,
            'lower_bound_ok': lower_ok,
            'upper_bound_ok': upper_ok,
            'ratio': ratio
        }


def benchmark_comparison(G: nx.Graph, k: int):
    """Compare original vs optimized implementation"""
    print(f"\n{'='*70}")
    print(f"Benchmarking on graph: n={G.number_of_nodes()}, m={G.number_of_edges()}, k={k}")
    print(f"{'='*70}")
    
    lsa = LargeSetArboricityOptimized(G)
    
    # Original algorithm
    print("\nOriginal Algorithm (O(n²) min-degree scan):")
    start = time.perf_counter()
    dk_orig, order_orig = lsa.modified_degeneracy_algorithm(k)
    time_orig = time.perf_counter() - start
    print(f"  dk = {dk_orig}")
    print(f"  Time: {time_orig:.4f}s")
    
    # Optimized algorithm
    print("\nOptimized Algorithm (O(m log n) heap-based):")
    start = time.perf_counter()
    dk_opt, order_opt = lsa.modified_degeneracy_algorithm_optimized(k)
    time_opt = time.perf_counter() - start
    print(f"  dk = {dk_opt}")
    print(f"  Time: {time_opt:.4f}s")
    
    # Speedup
    speedup = time_orig / time_opt if time_opt > 0 else float('inf')
    print(f"\nSpeedup: {speedup:.2f}x")
    print(f"Verification: {'✓ PASS' if dk_orig == dk_opt else '✗ FAIL'}")
    
    return {
        'time_original': time_orig,
        'time_optimized': time_opt,
        'speedup': speedup,
        'dk_value': dk_opt
    }


if __name__ == '__main__':
    import sys
    
    # Test on progressively larger graphs
    print("Testing on synthetic graphs...")
    
    test_sizes = [(100, 'n=100'), (500, 'n=500'), (1000, 'n=1000')]
    
    for n, label in test_sizes:
        print(f"\n{'#'*70}")
        print(f"# {label}")
        print(f"{'#'*70}")
        
        # Create Erdős-Rényi random graph
        G = nx.erdos_renyi_graph(n, 10/n)  # Average degree ~10
        k = max(1, n // 10)  # k = 10% of vertices
        
        benchmark_comparison(G, k)
    
    print("\n" + "="*70)
    print("To test with real graph, run:")
    print("  python large_set_arboricity_optimized.py <graph_file>")
    print("="*70)
