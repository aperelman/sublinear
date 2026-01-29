#!/usr/bin/env python3
"""
Large-Set-Arboricity - igraph + NumPy Implementation
Optimized for graphs larger than 100K nodes

Based on: "My notes for prove approximate for large-Set-Arboricity" by Amit Perelman
Uses igraph's C++ backend for graph operations + NumPy for vectorized computations
"""

import igraph as ig
import numpy as np
import heapq
import time
from typing import Tuple, List, Optional
from numba import njit


class LargeSetArboricityIgraph:
    """
    Fast implementation using igraph (C++ backend) + NumPy + Numba.
    
    The large-set-arboricity αk(G) is defined as:
        αk(G) = max_{G' ⊆ G, |V(G')| > k} ⌈d̄[G']⌉
    
    where d̄[G'] is the average degree of subgraph G'.
    """
    
    def __init__(self, G: ig.Graph):
        """Initialize with an igraph Graph."""
        self.G = G
        self.n = G.vcount()
        self.m = G.ecount()
    
    @classmethod
    def from_networkx(cls, G_nx):
        """
        Create from NetworkX graph with proper node ID mapping.
        
        Args:
            G_nx: NetworkX Graph
            
        Returns:
            LargeSetArboricityIgraph instance
        """
        # Get nodes and create mapping to contiguous IDs
        nodes = list(G_nx.nodes())
        n = len(nodes)
        node_to_idx = {node: i for i, node in enumerate(nodes)}
        
        # Convert edges using the mapping
        edges = list(G_nx.edges())
        edge_list = [(node_to_idx[u], node_to_idx[v]) for u, v in edges]
        
        # Create igraph with correct number of nodes
        G_ig = ig.Graph(n)
        if edge_list:
            G_ig.add_edges(edge_list)
        
        return cls(G_ig)
    
    @classmethod
    def from_edgelist(cls, edges: List[Tuple[int, int]], n: Optional[int] = None):
        """
        Create from edge list.
        
        Args:
            edges: List of (u, v) tuples
            n: Number of nodes (if None, inferred from edges)
            
        Returns:
            LargeSetArboricityIgraph instance
        """
        if n is None:
            n = max(max(u, v) for u, v in edges) + 1
        
        G = ig.Graph(n)
        if edges:
            G.add_edges(edges)
        
        return cls(G)
    
    def compute_dk(self, k: int, verbose: bool = False) -> int:
        """
        Compute dk(G) = αk(G) for a specific k using optimized heap-based algorithm.
        
        Args:
            k: Parameter (size of large set)
            verbose: Print progress information
            
        Returns:
            dk value (large-set-arboricity for parameter k)
        """
        n = self.n
        if k >= n:
            return 0
        if k < 0:
            k = 0
        
        # Get degree sequence as NumPy array (FAST: O(n) C++ operation)
        degrees = np.array(self.G.degree(), dtype=np.int32)
        
        # Build heap with (degree, vertex_id) pairs
        heap = [(degrees[v], v) for v in range(n)]
        heapq.heapify(heap)
        
        # Track removed vertices
        removed = np.zeros(n, dtype=bool)
        vertices_remaining = n
        max_avg_degree = 0.0
        
        # Current number of edges
        edges_remaining = self.m
        
        # Remove vertices one by one
        while heap and vertices_remaining > k:
            # Get minimum degree vertex
            deg, v = heapq.heappop(heap)
            
            # Skip if already removed (lazy deletion)
            if removed[v]:
                continue
            
            # Update max average degree BEFORE removing vertex
            if vertices_remaining > k:
                avg_degree = (2.0 * edges_remaining) / vertices_remaining
                max_avg_degree = max(max_avg_degree, avg_degree)
            
            # Mark as removed
            removed[v] = True
            vertices_remaining -= 1
            edges_remaining -= deg  # Remove edges incident to v
            
            # Update degrees of neighbors (FAST: igraph C++ neighbor lookup)
            neighbors = self.G.neighbors(v)
            for u in neighbors:
                if not removed[u]:
                    degrees[u] -= 1
                    # Re-insert with updated degree
                    heapq.heappush(heap, (degrees[u], u))
        
        dk_value = int(np.ceil(max_avg_degree))
        
        if verbose:
            print(f"d_{k}(G) = {dk_value}")
        
        return dk_value
    
    def compute_all_dk_optimized(self, verbose: bool = True) -> Tuple[np.ndarray, np.ndarray]:
        """
        OPTIMIZED: Compute dk(G) for ALL k values (0 to n-1) in single pass.
        
        Strategy:
        1. Run degeneracy ordering ONCE
        2. Track graph state at each removal step
        3. For each k, find max average degree seen when vertices > k
        
        This is O(n + m) with the degeneracy ordering, then O(n²) for computing all dk.
        But the O(n²) part uses NumPy vectorization so it's very fast.
        
        Args:
            verbose: Print progress information
            
        Returns:
            (k_values, dk_values) as NumPy arrays
        """
        n = self.n
        
        if verbose:
            print(f"Computing all d_k values for graph with n={n}, m={self.m}...")
            start_time = time.time()
        
        # Get degree sequence as NumPy array
        degrees = np.array(self.G.degree(), dtype=np.int32)
        
        # Build heap
        heap = [(degrees[v], v) for v in range(n)]
        heapq.heapify(heap)
        
        # Track state at each removal step
        removed = np.zeros(n, dtype=bool)
        vertices_at_step = np.zeros(n, dtype=np.int32)
        edges_at_step = np.zeros(n, dtype=np.int32)
        
        vertices_remaining = n
        edges_remaining = self.m
        step = 0
        
        # Record initial state
        vertices_at_step[0] = vertices_remaining
        edges_at_step[0] = edges_remaining
        
        # Remove vertices one by one
        while heap and step < n - 1:
            deg, v = heapq.heappop(heap)
            
            if removed[v]:
                continue
            
            # Remove vertex
            removed[v] = True
            vertices_remaining -= 1
            edges_remaining -= deg
            
            # Update neighbor degrees
            neighbors = self.G.neighbors(v)
            for u in neighbors:
                if not removed[u]:
                    degrees[u] -= 1
                    heapq.heappush(heap, (degrees[u], u))
            
            # Record state after removal
            step += 1
            vertices_at_step[step] = vertices_remaining
            edges_at_step[step] = edges_remaining
        
        # Now compute all dk values using recorded states
        dk_values = _compute_dk_from_states(
            vertices_at_step[:step+1],
            edges_at_step[:step+1],
            n
        )
        
        if verbose:
            elapsed = time.time() - start_time
            print(f"✓ Computed all d_k values in {elapsed:.3f} seconds")
            print(f"  Degeneracy d_0 = {dk_values[0]}")
            print(f"  Arboricity α(G) ≈ ⌈d_0/2⌉ = {int(np.ceil(dk_values[0]/2))}")
        
        k_values = np.arange(n, dtype=np.int32)
        return k_values, dk_values
    
    def compute_arboricity_bound(self) -> int:
        """
        Compute upper bound on arboricity: α(G) ≤ ⌈degeneracy/2⌉
        
        Returns:
            Upper bound on arboricity
        """
        degeneracy = self.compute_dk(0, verbose=False)
        return int(np.ceil(degeneracy / 2))
    
    def analyze_graph(self) -> dict:
        """
        Complete graph analysis including all dk values.
        
        Returns:
            Dictionary with analysis results
        """
        print(f"\n{'='*60}")
        print(f"Graph Analysis")
        print(f"{'='*60}")
        print(f"Nodes: {self.n:,}")
        print(f"Edges: {self.m:,}")
        
        if self.n > 1:
            density = 2*self.m/(self.n*(self.n-1))
            print(f"Density: {density:.6f}")
        print(f"{'='*60}\n")
        
        # Compute all dk values
        k_values, dk_values = self.compute_all_dk_optimized(verbose=True)
        
        # Key values
        degeneracy = int(dk_values[0])
        arboricity_bound = int(np.ceil(degeneracy / 2))
        
        print(f"\n{'='*60}")
        print(f"Key Results:")
        print(f"  Degeneracy d_0(G) = {degeneracy}")
        print(f"  Arboricity bound α(G) ≤ {arboricity_bound}")
        print(f"{'='*60}\n")
        
        return {
            'n': self.n,
            'm': self.m,
            'k_values': k_values,
            'dk_values': dk_values,
            'degeneracy': degeneracy,
            'arboricity_bound': arboricity_bound
        }


@njit
def _compute_dk_from_states(vertices_at_step: np.ndarray, 
                            edges_at_step: np.ndarray,
                            n: int) -> np.ndarray:
    """
    Compute all dk values from recorded states.
    Compiled with Numba for speed.
    
    For each k, we want:
        dk = max over all steps where vertices_remaining > k of:
             ceil(2 * edges / vertices)
    
    Args:
        vertices_at_step: Array of vertex counts at each step
        edges_at_step: Array of edge counts at each step
        n: Total number of vertices
        
    Returns:
        Array of dk values for k=0 to n-1
    """
    num_steps = len(vertices_at_step)
    dk_values = np.zeros(n, dtype=np.int32)
    
    for k in range(n):
        max_avg_degree = 0.0
        
        # Find max average degree among steps where vertices > k
        for step in range(num_steps):
            vertices = vertices_at_step[step]
            edges = edges_at_step[step]
            
            if vertices > k and vertices > 0:
                avg_degree = (2.0 * edges) / vertices
                if avg_degree > max_avg_degree:
                    max_avg_degree = avg_degree
        
        dk_values[k] = int(np.ceil(max_avg_degree))
    
    return dk_values


def main():
    """Example usage and testing"""
    print("Large-Set-Arboricity (igraph + Numba implementation)\n")
    
    # Example 1: Small test graph
    print("Example 1: Small test graph")
    edges = [(0,1), (1,2), (2,3), (3,0), (0,2), (1,3)]
    lsa = LargeSetArboricityIgraph.from_edgelist(edges, n=4)
    results = lsa.analyze_graph()
    
    # Example 2: Larger random graph
    print("\n" + "="*60)
    print("Example 2: Erdős-Rényi random graph")
    print("="*60)
    
    # Create random graph with igraph
    G_random = ig.Graph.Erdos_Renyi(n=1000, p=0.01)
    lsa_random = LargeSetArboricityIgraph(G_random)
    results_random = lsa_random.analyze_graph()
    
    # Verify correctness: compare compute_dk(0) with optimized version
    print("\n" + "="*60)
    print("Verification: Comparing methods")
    print("="*60)
    
    single_d0 = lsa_random.compute_dk(0, verbose=False)
    all_dk = results_random['dk_values']
    
    print(f"compute_dk(0) = {single_d0}")
    print(f"compute_all_dk_optimized()[0] = {all_dk[0]}")
    print(f"Match: {single_d0 == all_dk[0]} ✓" if single_d0 == all_dk[0] else f"MISMATCH! ✗")
    
    print("\n✓ Examples completed successfully!")


if __name__ == "__main__":
    main()