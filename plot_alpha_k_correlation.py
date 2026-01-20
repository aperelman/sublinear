#!/usr/bin/env python3
"""
Plot α_k vs k correlation
Easy configuration at the top of the file!
"""

import os
import sys
import matplotlib.pyplot as plt
import networkx as nx
from snap_api import load_snap_graph
from large_set_arboricity import LargeSetArboricity


# ============================================================================
# CONFIGURATION - EASY TO CHANGE!
# ============================================================================
GRAPH_TYPE = 'snap'          # 'snap', 'synthetic', or 'file'
GRAPH_NAME = 'email-Enron'   # For snap: 'ca-GrQc', 'email-Enron', etc.
MAX_K = 50                   # Maximum k value to compute
# ============================================================================


def load_graph():
    """Load graph based on configuration"""
    if GRAPH_TYPE == 'snap':
        print(f"Loading SNAP graph: {GRAPH_NAME}...")
        G = load_snap_graph(GRAPH_NAME)
        return G
    
    elif GRAPH_TYPE == 'synthetic':
        print(f"Creating synthetic graph: {GRAPH_NAME}...")
        if GRAPH_NAME == 'petersen':
            return nx.petersen_graph()
        elif GRAPH_NAME == 'complete':
            return nx.complete_graph(10)
        elif GRAPH_NAME == 'cycle':
            return nx.cycle_graph(15)
        else:
            print(f"Unknown synthetic graph: {GRAPH_NAME}")
            sys.exit(1)
    
    elif GRAPH_TYPE == 'file':
        print(f"Loading graph from file: {GRAPH_NAME}...")
        G = nx.read_edgelist(GRAPH_NAME, nodetype=int)
        return G
    
    else:
        print(f"Unknown graph type: {GRAPH_TYPE}")
        sys.exit(1)


def create_plot(G):
    """Create the α_k vs k correlation plot"""
    n = G.number_of_nodes()
    m = G.number_of_edges()
    
    print(f"Graph loaded: n={n:,} nodes, m={m:,} edges")
    
    # Determine if we can compute exact αk
    can_compute_alpha = (n <= 15)
    
    if can_compute_alpha:
        print(f"Graph is small (n={n} ≤ 15), will compute EXACT αk(G)")
    else:
        print(f"Graph is large (n={n} > 15), will only compute dk(G) approximation")
    
    # Initialize
    lsa = LargeSetArboricity(G)
    
    # Compute for k values
    max_k = min(MAX_K, n - 1)
    k_values = list(range(max_k + 1))
    
    print(f"\nComputing dk(G) for k=0 to {max_k}...")
    
    dk_values = []
    alpha_values = []
    
    for i, k in enumerate(k_values):
        # Always compute dk
        dk_G, _ = lsa.modified_degeneracy_algorithm(k)
        dk_values.append(dk_G)
        
        # Compute exact αk if possible
        if can_compute_alpha:
            alpha_k, _ = lsa.compute_alpha_k_exact(k)
            alpha_values.append(alpha_k)
        
        # Progress
        if (i + 1) % 10 == 0 or i == len(k_values) - 1:
            print(f"  Progress: {i+1}/{len(k_values)}")
    
    print(f"✓ Computation complete")
    
    # Create plot
    fig, ax = plt.subplots(figsize=(12, 7))
    
    if can_compute_alpha:
        # Plot exact αk
        ax.plot(k_values, alpha_values, 'ro-', linewidth=2.5, 
                markersize=8, label='αk(G) - exact', 
                markevery=max(1, len(k_values)//20))
        
        # Plot dk for comparison
        ax.plot(k_values, dk_values, 'bs--', linewidth=2, 
                markersize=6, label='dk(G) - approximation',
                markevery=max(1, len(k_values)//20))
        
        # Approximation bounds
        ax.fill_between(k_values, dk_values, 
                        [2*d for d in dk_values],
                        alpha=0.2, color='green',
                        label='2-approx bounds [dk, 2dk]')
    else:
        # Only dk available
        ax.plot(k_values, dk_values, 'bs-', linewidth=2.5, 
                markersize=8, label='dk(G) - approximation',
                markevery=max(1, len(k_values)//20))
        
        # Approximation bounds
        ax.fill_between(k_values, dk_values, 
                        [2*d for d in dk_values],
                        alpha=0.2, color='green',
                        label='2-approx bounds [dk, 2dk]')
    
    # Formatting
    ax.set_xlabel('k (parameter)', fontsize=13, fontweight='bold')
    ax.set_ylabel('Value', fontsize=13, fontweight='bold')
    
    title = f'{GRAPH_NAME}\n'
    title += f'n={n:,} nodes, m={m:,} edges, avg degree={2*m/n:.2f}'
    if can_compute_alpha:
        title += '\nEXACT αk(G) vs dk(G)'
    else:
        title += '\ndk(G) approximation only (graph too large for exact αk)'
    
    ax.set_title(title, fontsize=14, fontweight='bold')
    ax.legend(fontsize=11, loc='best')
    ax.grid(True, alpha=0.3)
    
    # Stats box
    stats = f"Max dk: {max(dk_values)}\n"
    if can_compute_alpha and alpha_values:
        stats += f"Max αk: {max(alpha_values)}\n"
        stats += f"Ratio: {max(alpha_values)/max(dk_values):.2f}"
    else:
        stats += f"⇒ αk ∈ [{max(dk_values)}, {2*max(dk_values)}]"
    
    ax.text(0.02, 0.98, stats, transform=ax.transAxes, fontsize=10,
            verticalalignment='top',
            bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.8))
    
    plt.tight_layout()
    
    # Save
    output_file = 'alpha_k_vs_k_comparison.png'
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    
    abs_path = os.path.abspath(output_file)
    print(f"\n✓ Saved plot to: {output_file}")
    print(f"  Full path: {abs_path}")
    
    plt.close()
    
    return output_file


def main():
    print("\n" + "="*70)
    print("αk vs k Analysis")
    print("="*70)
    print(f"\nConfiguration:")
    print(f"  Graph type: {GRAPH_TYPE}")
    print(f"  Graph name: {GRAPH_NAME}")
    print(f"  Max k: {MAX_K}")
    print("="*70 + "\n")
    
    try:
        # Load graph
        G = load_graph()
        
        # Create plot
        output_file = create_plot(G)
        
        print("\n" + "="*70)
        print("✅ SUCCESS!")
        print("="*70)
        print(f"\nPlot saved: {output_file}")
        print("="*70 + "\n")
        
    except Exception as e:
        print(f"\n❌ ERROR: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == '__main__':
    main()
