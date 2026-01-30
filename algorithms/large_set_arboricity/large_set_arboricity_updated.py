#!/usr/bin/env python3
"""
Large-Set-Arboricity Algorithms
Based on: "My notes for prove approximate for large-Set-Arboricity" by Amit Perelman
"""

import networkx as nx
import math
import matplotlib.pyplot as plt
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
    
    def compute_alpha_k_removal(self, k: int) -> Tuple[int, Optional[nx.Graph]]:
        """
        Compute αk(G) using the removal algorithm from the PDF
        Algorithm: Remove n-k vertices (minimum degree first), track max average degree
        
        Args:
            k: Parameter (size of large set)
        
        Returns:
            (alpha_k_value, best_subgraph)
        """
        n = self.n
        if k >= n:
            # If k >= n, return average degree of entire graph
            if self.G.number_of_edges() == 0:
                return 0, None
            avg_deg = 2 * self.G.number_of_edges() / n
            return math.ceil(avg_deg), self.G.copy()
        
        if k <= 0:
            return 0, None
        
        H = self.G.copy()
        max_alpha = 0
        best_subgraph = None
        
        # Remove n-k vertices (one at a time, minimum degree first)
        num_removals = n - k
        
        for step in range(num_removals):
            if H.number_of_nodes() == 0:
                break
            
            # Compute average degree of current subgraph
            if H.number_of_nodes() > k and H.number_of_edges() > 0:
                avg_deg = 2 * H.number_of_edges() / H.number_of_nodes()
                alpha_val = math.ceil(avg_deg)
                if alpha_val > max_alpha:
                    max_alpha = alpha_val
                    best_subgraph = H.copy()
            
            # Find and remove minimum degree vertex
            min_deg = float('inf')
            min_vertex = None
            for v in H.nodes():
                deg = H.degree(v)
                if deg < min_deg:
                    min_deg = deg
                    min_vertex = v
            
            H.remove_node(min_vertex)
        
        # Check final subgraph (k vertices remaining)
        if H.number_of_nodes() > 0 and H.number_of_edges() > 0:
            avg_deg = 2 * H.number_of_edges() / H.number_of_nodes()
            alpha_val = math.ceil(avg_deg)
            if alpha_val > max_alpha:
                max_alpha = alpha_val
                best_subgraph = H.copy()
        
        return max_alpha, best_subgraph
    
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
        alpha_k, _ = self.compute_alpha_k_removal(k)
        
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
    
    def plot_alpha_k_vs_k(self, k_range: Optional[List[int]] = None, 
                          save_path: Optional[str] = None):
        """
        Plot αk(G) and dk(G) as functions of k
        
        Args:
            k_range: List of k values to plot (default: 1 to n-1)
            save_path: Path to save the plot (optional)
        """
        if k_range is None:
            k_range = list(range(1, self.n))
        
        alpha_values = []
        dk_values = []
        
        print(f"Computing αk and dk for k in {min(k_range)} to {max(k_range)}...")
        for k in k_range:
            alpha_k, _ = self.compute_alpha_k_removal(k)
            dk, _ = self.modified_degeneracy_algorithm(k)
            alpha_values.append(alpha_k)
            dk_values.append(dk)
            print(f"  k={k}: αk={alpha_k}, dk={dk}")
        
        # Create plot
        plt.figure(figsize=(10, 6))
        plt.plot(k_range, alpha_values, 'b-o', label='αk(G)', linewidth=2, markersize=6)
        plt.plot(k_range, dk_values, 'r--s', label='dk(G)', linewidth=2, markersize=6)
        plt.plot(k_range, [2*d for d in dk_values], 'g:', label='2·dk(G)', linewidth=1.5)
        
        plt.xlabel('k (large set size)', fontsize=12)
        plt.ylabel('Value', fontsize=12)
        plt.title(f'Large-Set-Arboricity: αk(G) vs k\n(n={self.n}, m={self.G.number_of_edges()})', 
                  fontsize=14)
        plt.legend(fontsize=11)
        plt.grid(True, alpha=0.3)
        
        # Add verification annotations
        plt.text(0.02, 0.98, 
                f'Graph: {self.n} nodes, {self.G.number_of_edges()} edges',
                transform=plt.gca().transAxes,
                verticalalignment='top',
                bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5),
                fontsize=10)
        
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
            print(f"Plot saved to {save_path}")
        
        plt.show()


def create_test_graphs():
    """Create various test graphs"""
    graphs = {
        'K6': nx.complete_graph(6),
        'K10': nx.complete_graph(10),
        'Path10': nx.path_graph(10),
        'Cycle8': nx.cycle_graph(8),
        'Star8': nx.star_graph(8),
        'ER(20,0.3)': nx.erdos_renyi_graph(20, 0.3),
        'BA(20,3)': nx.barabasi_albert_graph(20, 3),
        'Grid3x3': nx.grid_2d_graph(3, 3),
    }
    return graphs


if __name__ == '__main__':
    print("=" * 60)
    print("Large-Set-Arboricity Analysis")
    print("=" * 60)
    
    # Test 1: Small complete graph
    print("\n### Test 1: Complete graph K6 ###")
    G = nx.complete_graph(6)
    lsa = LargeSetArboricity(G)
    
    for k in range(1, 6):
        result = lsa.verify_approximation(k)
        print(f"k={k}: dk={result['dk']}, αk={result['alpha_k']}, "
              f"ratio={result['ratio']:.2f}, bounds OK: {result['lower_bound_ok'] and result['upper_bound_ok']}")
    
    # Plot for K6
    print("\nGenerating plot for K6...")
    lsa.plot_alpha_k_vs_k(save_path='alpha_k_plot_K6.png')
    
    # Test 2: Erdos-Renyi random graph
    print("\n### Test 2: Erdos-Renyi G(20, 0.3) ###")
    G = nx.erdos_renyi_graph(20, 0.3)
    lsa = LargeSetArboricity(G)
    
    # Sample a few k values
    for k in [1, 5, 10, 15, 19]:
        result = lsa.verify_approximation(k)
        print(f"k={k}: dk={result['dk']}, αk={result['alpha_k']}, "
              f"ratio={result['ratio']:.2f}")
    
    # Plot for ER graph
    print("\nGenerating plot for Erdos-Renyi graph...")
    lsa.plot_alpha_k_vs_k(save_path='alpha_k_plot_ER.png')
    
    # Test 3: Barabasi-Albert graph
    print("\n### Test 3: Barabasi-Albert BA(25, 3) ###")
    G = nx.barabasi_albert_graph(25, 3)
    lsa = LargeSetArboricity(G)
    
    print("\nGenerating plot for Barabasi-Albert graph...")
    lsa.plot_alpha_k_vs_k(k_range=list(range(1, 25, 2)), save_path='alpha_k_plot_BA.png')
    
    print("\n" + "=" * 60)
    print("Analysis complete! Check the generated PNG files.")
    print("=" * 60)
