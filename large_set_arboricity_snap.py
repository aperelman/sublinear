#!/usr/bin/env python3
"""
Large-Set-Arboricity Algorithms with SNAP Graph Support
Based on: "My notes for prove approximate for large-Set-Arboricity" by Amit Perelman
"""

import networkx as nx
import math
import matplotlib.pyplot as plt
from typing import Tuple, List, Optional
from itertools import combinations
import urllib.request
import gzip
import os


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
        self.m = G.number_of_edges()
    
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
    
    def compute_alpha_k_removal(self, k: int, verbose: bool = False) -> Tuple[int, Optional[nx.Graph]]:
        """
        Compute αk(G) using the removal algorithm from the PDF
        Algorithm: Remove n-k vertices (minimum degree first), track max average degree
        
        Args:
            k: Parameter (size of large set)
            verbose: Print progress for large graphs
        
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
            
            if verbose and step % 1000 == 0:
                print(f"  Removal step {step}/{num_removals}, current nodes: {H.number_of_nodes()}")
            
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
    
    def verify_approximation(self, k: int, verbose: bool = False) -> dict:
        """
        Verify that dk(G) ≤ αk(G) ≤ 2·dk(G)
        
        Returns:
            Dictionary with verification results
        """
        dk_value, _ = self.modified_degeneracy_algorithm(k)
        alpha_k, _ = self.compute_alpha_k_removal(k, verbose=verbose)
        
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
                          save_path: Optional[str] = None,
                          title_suffix: str = ""):
        """
        Plot αk(G) and dk(G) as functions of k
        
        Args:
            k_range: List of k values to plot (default: sampled range)
            save_path: Path to save the plot (optional)
            title_suffix: Additional text for plot title
        """
        if k_range is None:
            # For large graphs, sample k values
            if self.n > 100:
                step = max(1, self.n // 50)
                k_range = list(range(1, self.n, step))
            else:
                k_range = list(range(1, self.n))
        
        alpha_values = []
        dk_values = []
        
        print(f"Computing αk and dk for {len(k_range)} values of k...")
        for i, k in enumerate(k_range):
            if i % 10 == 0:
                print(f"  Progress: {i}/{len(k_range)} (k={k})")
            alpha_k, _ = self.compute_alpha_k_removal(k, verbose=False)
            dk, _ = self.modified_degeneracy_algorithm(k)
            alpha_values.append(alpha_k)
            dk_values.append(dk)
        
        print(f"Computation complete!")
        
        # Create plot
        plt.figure(figsize=(12, 7))
        plt.plot(k_range, alpha_values, 'b-o', label='αk(G)', linewidth=2, markersize=4)
        plt.plot(k_range, dk_values, 'r--s', label='dk(G)', linewidth=2, markersize=4)
        plt.plot(k_range, [2*d for d in dk_values], 'g:', label='2·dk(G)', linewidth=1.5)
        
        plt.xlabel('k (large set size)', fontsize=12)
        plt.ylabel('Value', fontsize=12)
        title = f'Large-Set-Arboricity: αk(G) vs k'
        if title_suffix:
            title += f' - {title_suffix}'
        title += f'\n(n={self.n}, m={self.m})'
        plt.title(title, fontsize=14)
        plt.legend(fontsize=11)
        plt.grid(True, alpha=0.3)
        
        # Add info box
        info_text = f'Dataset: {title_suffix}\n' if title_suffix else ''
        info_text += f'Graph: {self.n:,} nodes, {self.m:,} edges\n'
        info_text += f'Avg degree: {2*self.m/self.n:.2f}\n'
        info_text += f'k range: [{min(k_range)}, {max(k_range)}]'
        plt.text(0.02, 0.98, info_text,
                transform=plt.gca().transAxes,
                verticalalignment='top',
                bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.7),
                fontsize=10)
        
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
            print(f"Plot saved to {save_path}")
        
        plt.show()


def download_snap_graph(dataset_name: str, data_dir: str = 'snap_data') -> str:
    """
    Download a SNAP graph dataset
    
    Args:
        dataset_name: Name of the dataset (e.g., 'CA-GrQc')
        data_dir: Directory to save the data
    
    Returns:
        Path to the downloaded file
    """
    # Create data directory if it doesn't exist
    os.makedirs(data_dir, exist_ok=True)
    
    # SNAP dataset URLs
    snap_urls = {
        'CA-GrQc': 'https://snap.stanford.edu/data/ca-GrQc.txt.gz',
        'CA-HepTh': 'https://snap.stanford.edu/data/ca-HepTh.txt.gz',
        'CA-HepPh': 'https://snap.stanford.edu/data/ca-HepPh.txt.gz',
        'email-Enron': 'https://snap.stanford.edu/data/email-Enron.txt.gz',
    }
    
    if dataset_name not in snap_urls:
        raise ValueError(f"Dataset {dataset_name} not found. Available: {list(snap_urls.keys())}")
    
    url = snap_urls[dataset_name]
    gz_file = os.path.join(data_dir, f'{dataset_name}.txt.gz')
    txt_file = os.path.join(data_dir, f'{dataset_name}.txt')
    
    # Check if already downloaded
    if os.path.exists(txt_file):
        print(f"Dataset {dataset_name} already exists at {txt_file}")
        return txt_file
    
    # Download
    print(f"Downloading {dataset_name} from SNAP...")
    urllib.request.urlretrieve(url, gz_file)
    print(f"Downloaded to {gz_file}")
    
    # Decompress
    print(f"Decompressing...")
    with gzip.open(gz_file, 'rb') as f_in:
        with open(txt_file, 'wb') as f_out:
            f_out.write(f_in.read())
    
    # Remove .gz file
    os.remove(gz_file)
    print(f"Dataset ready at {txt_file}")
    
    return txt_file


def load_snap_graph(filepath: str) -> nx.Graph:
    """
    Load a SNAP graph from edge list file
    
    Args:
        filepath: Path to the edge list file
    
    Returns:
        NetworkX graph
    """
    print(f"Loading graph from {filepath}...")
    
    # SNAP files have comments starting with #
    G = nx.read_edgelist(filepath, comments='#', nodetype=int)
    
    # Remove self-loops
    G.remove_edges_from(nx.selfloop_edges(G))
    
    # Get largest connected component (common practice)
    if not nx.is_connected(G):
        print("Graph is not connected, extracting largest component...")
        components = list(nx.connected_components(G))
        largest = max(components, key=len)
        G = G.subgraph(largest).copy()
    
    print(f"Loaded graph: {G.number_of_nodes()} nodes, {G.number_of_edges()} edges")
    
    return G


def analyze_snap_graph(dataset_name: str = 'CA-GrQc', 
                       k_samples: int = 30,
                       k_min: Optional[int] = None,
                       k_max: Optional[int] = None,
                       k_step: int = 1):
    """
    Download, load, and analyze a SNAP graph
    
    Args:
        dataset_name: SNAP dataset name
        k_samples: Number of k values to sample for plotting (ignored if k_min/k_max specified)
        k_min: Minimum k value for sequential range (default: n//100)
        k_max: Maximum k value for sequential range (default: n-1)
        k_step: Step size for sequential k values (default: 1 for fully sequential)
    """
    print("=" * 70)
    print(f"SNAP Graph Analysis: {dataset_name}")
    print("=" * 70)
    
    # Download and load
    filepath = download_snap_graph(dataset_name)
    G = load_snap_graph(filepath)
    
    # Create analyzer
    lsa = LargeSetArboricity(G)
    
    # Basic graph stats
    print(f"\nGraph Statistics:")
    print(f"  Nodes: {lsa.n:,}")
    print(f"  Edges: {lsa.m:,}")
    print(f"  Average degree: {2*lsa.m/lsa.n:.2f}")
    print(f"  Density: {nx.density(G):.6f}")
    
    # Sample k values for analysis
    k_values = [lsa.n // 10, lsa.n // 4, lsa.n // 2, 3*lsa.n // 4]
    k_values = [k for k in k_values if k > 0 and k < lsa.n]
    
    print(f"\nComputing αk for selected k values:")
    for k in k_values:
        result = lsa.verify_approximation(k, verbose=True)
        print(f"  k={k:,}: dk={result['dk']}, αk={result['alpha_k']}, "
              f"ratio={result['ratio']:.3f}, bounds OK: "
              f"{result['lower_bound_ok'] and result['upper_bound_ok']}")
    
    # Create plot with specified k range
    print(f"\nGenerating plot...")
    
    # Use sequential range if k_min/k_max specified
    if k_min is not None or k_max is not None:
        if k_min is None:
            k_min = max(1, lsa.n // 100)
        if k_max is None:
            k_max = lsa.n - 1
        
        k_range = list(range(k_min, min(k_max + 1, lsa.n), k_step))
        print(f"Using sequential k range: {k_min} to {k_max} (step={k_step})")
        print(f"Total k values: {len(k_range)}")
    else:
        # Use sampled points
        step = max(1, lsa.n // k_samples)
        k_range = list(range(step, lsa.n, step))
        print(f"Using {len(k_range)} sampled k values")
    
    save_path = f'alpha_k_plot_{dataset_name}.png'
    lsa.plot_alpha_k_vs_k(k_range=k_range, save_path=save_path, title_suffix=dataset_name)
    
    print("\n" + "=" * 70)
    print("Analysis complete!")
    print("=" * 70)


if __name__ == '__main__':
    # Example 1: Default - sampled k values (fast)
    # analyze_snap_graph('CA-GrQc', k_samples=50)
    
    # Example 2: Sequential k from 1 to 100 (fully sequential, small range)
    analyze_snap_graph('CA-GrQc', k_min=1, k_max=100, k_step=1)
    
    # Example 3: Sequential k from 100 to 1000, step by 10
    # analyze_snap_graph('CA-GrQc', k_min=100, k_max=1000, k_step=10)
    
    # Example 4: Sequential k from 1 to 500, step by 5
    # analyze_snap_graph('CA-GrQc', k_min=1, k_max=500, k_step=5)
    
    # Example 5: All k values from 1 to n-1 (WARNING: very slow for large graphs!)
    # analyze_snap_graph('CA-GrQc', k_min=1, k_max=None, k_step=1)
    
    # Uncomment to analyze other datasets:
    # analyze_snap_graph('CA-HepTh', k_min=1, k_max=200, k_step=2)
    # analyze_snap_graph('email-Enron', k_samples=30)
