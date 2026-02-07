#!/usr/bin/env python3
"""
Simple Main Program: Analyze SNAP Graph

Usage:
    python main_simple.py [graph_name]
    
Example:
    python main_simple.py ca-GrQc
"""

import sys
from snap_api import load_snap_graph
from large_set_arboricity import LargeSetArboricity
from plot_alpha_k import plot_alpha_k_vs_k


def main():
    # Get graph name from command line or use default
    if len(sys.argv) > 1:
        graph_name = sys.argv[1]
    else:
        graph_name = 'ca-GrQc'  # Default
        print(f"No graph specified, using default: {graph_name}")
        print(f"Usage: python main_simple.py <graph_name>")
        print()
    
    print("="*60)
    print(f"ANALYZING: {graph_name}")
    print("="*60)
    
    # Load graph
    print(f"\n1. Loading graph...")
    G = load_snap_graph(graph_name)
    
    n = G.number_of_nodes()
    m = G.number_of_edges()
    
    print(f"\nGraph properties:")
    print(f"   Nodes: {n}")
    print(f"   Edges: {m}")
    print(f"   Avg degree: {2*m/n:.2f}")
    
    # Initialize
    lsa = LargeSetArboricity(G)
    
    # Compute for different k values
    print(f"\n2. Computing dk(G) and αk(G)...")
    print()
    print(f"{'k':<5} {'dk(G)':<10} {'αk(G)':<10} {'Ratio':<10} {'Bounds OK?'}")
    print("-" * 60)
    
    max_k = min(n - 1, 10)  # Analyze k from 0 to 10
    
    for k in range(max_k + 1):
        # Compute dk(G)
        dk_G, _ = lsa.modified_degeneracy_algorithm(k)
        
        # Compute αk(G) if small enough
        if n <= 15:
            alpha_k, _ = lsa.compute_alpha_k_exact(k)
            ratio = alpha_k / dk_G if dk_G > 0 else float('inf')
            
            # Check bounds
            lower_ok = dk_G <= alpha_k
            upper_ok = alpha_k <= 2 * dk_G
            bounds_ok = '✓' if (lower_ok and upper_ok) else '✗'
            
            print(f"{k:<5} {dk_G:<10} {alpha_k:<10} {ratio:<10.3f} {bounds_ok}")
        else:
            print(f"{k:<5} {dk_G:<10} {'N/A':<10} {'N/A':<10} {'N/A'}")
    
    # Generate plot
    if n <= 15:
        print(f"\n3. Generating plot...")
        plot_alpha_k_vs_k(G, graph_name, max_k=max_k, 
                         save_path=f"{graph_name}_plot.png")
        print(f"   Saved to: {graph_name}_plot.png")
    else:
        print(f"\n3. Graph too large (n={n}) for exact computation and plotting.")
        print(f"   Only dk(G) approximation computed.")
    
    print("\n" + "="*60)
    print("DONE! ✅")
    print("="*60)


if __name__ == "__main__":
    main()
