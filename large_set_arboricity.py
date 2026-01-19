#!/usr/bin/env python3
"""
Large-Set-Arboricity Algorithms
Based on: "My notes for prove approximate for large-Set-Arboricity" by Amit Perelman
"""

import networkx as nx
import math
from typing import Tuple, List, Optional
from itertools import combinations


class LargeSetArboricity:
    """
    Implements algorithms for computing and approximating large-set-arboricity.
    
    The large-set-arboricity αk(G) is defined as:
        αk(G) = max_{G' ⊆ G, |V(G')| > k} ⌈d̄[G']⌉
    
    where d̄[G'] is the average degree of subgraph G'.
    """
    
    def __init__(self, G: nx.Graph):
        """Initialize with a NetworkX graph"""
        self.G = G.copy()
        self.n = G.number_of_nodes()
    
    def modified_degeneracy_algorithm(self, k: int) -> Tuple[int, List[int]]:
        """
        Modified Degeneracy Algorithm
        Computes dk(G) = max degree in last k vertices removed
        
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
        
        H = self.G.copy()
        removal_order = []
        degree_at_removal = {}
        
        # Remove vertices one by one (minimum degree first)
        for step in range(n):
            if H.number_of_nodes() == 0:
                break
            
            # Find minimum degree vertex
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
        
        Args:
            k: Parameter
        
        Returns:
            (alpha_k_value, best_subgraph)
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
    
    def verify_approximation(self, k: int) -> dict:
        """
        Verify that dk(G) ≤ αk(G) ≤ 2·dk(G)
        
        Returns:
            Dictionary with verification results
        """
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


if __name__ == '__main__':
    # Test on a small graph
    G = nx.complete_graph(6)
    lsa = LargeSetArboricity(G)
    
    print("Testing on K6 (complete graph with 6 nodes)")
    for k in range(1, 6):
        result = lsa.verify_approximation(k)
        print(f"k={k}: dk={result['dk']}, αk={result['alpha_k']}, ratio={result['ratio']:.2f}")
