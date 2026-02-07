#!/usr/bin/env python3
"""
Main Program: Large-Set-Arboricity Analysis on SNAP Graphs

This program:
1. Loads small SNAP graphs
2. Computes dk(G) and αk(G) for different k values
3. Prints detailed results
4. Generates plots showing correlation between k, dk(G), and αk(G)

Author: Based on "My notes for prove approximate for large-Set-Arboricity" by Amit Perelman
"""

import networkx as nx
import matplotlib.pyplot as plt
import numpy as np
from tabulate import tabulate
import sys

# Import our implementations
try:
    from snap_api import load_snap_graph, SNAPLoader
    from large_set_arboricity import LargeSetArboricity
    from plot_alpha_k import plot_alpha_k_vs_k, compute_alpha_k_for_all_k
except ImportError as e:
    print(f"Error importing modules: {e}")
    print("Make sure snap_api.py, large_set_arboricity.py, and plot_alpha_k.py are in the same directory")
    sys.exit(1)


def analyze_graph_dk_only(G, graph_name, max_k=None):
    """
    Analyze large graphs - compute only dk(G) approximation
    
    Args:
        G: NetworkX graph
        graph_name: Name for display
        max_k: Maximum k value (default: min(n-1, 50))
    """
    n = G.number_of_nodes()
    m = G.number_of_edges()
    
    # Initialize
    lsa = LargeSetArboricity(G)
    
    # Determine k range (limit for visualization)
    if max_k is None:
        max_k = min(n - 1, 50)
    else:
        max_k = min(max_k, n - 1)
    
    print(f"\n🔬 COMPUTING dk(G) FOR k = 0 TO {max_k}...")
    
    k_values = []
    dk_values = []
    
    for k in range(max_k + 1):
        dk_G, removal_seq = lsa.modified_degeneracy_algorithm(k)
        k_values.append(k)
        dk_values.append(dk_G)
        
        if k <= 5 or k % 10 == 0 or k == max_k:
            print(f"   k={k:3d}: dk(G) = {dk_G}")
    
    # Print summary
    print(f"\n📈 SUMMARY (dk only):")
    print(f"   Minimum dk:  {min(dk_values)}")
    print(f"   Maximum dk:  {max(dk_values)}")
    print(f"   Final dk:    {dk_values[-1]}")
    
    # Create plot
    create_dk_only_plot(k_values, dk_values, graph_name, n, m)
    
    return {
        'k_values': k_values,
        'dk_values': dk_values,
        'graph_name': graph_name,
        'dk_only': True
    }


def create_dk_only_plot(k_values, dk_values, graph_name, n, m):
    """Create plot showing dk(G) behavior for large graphs"""
    fig, axes = plt.subplots(1, 2, figsize=(14, 6))
    fig.suptitle(f'Large-Set-Arboricity (dk approximation): {graph_name}\n'
                 f'n={n} nodes, m={m} edges',
                 fontsize=14, fontweight='bold')
    
    # Plot 1: dk(G) vs k
    ax1 = axes[0]
    ax1.plot(k_values, dk_values, 'b-o', linewidth=2, markersize=4, 
             markevery=max(1, len(k_values)//20))
    ax1.set_xlabel('k (parameter)', fontsize=11)
    ax1.set_ylabel('dk(G)', fontsize=11)
    ax1.set_title('dk(G) vs k (approximation)', fontsize=12, fontweight='bold')
    ax1.grid(True, alpha=0.3)
    
    # Add stats
    stats = f"Min dk: {min(dk_values)}\n"
    stats += f"Max dk: {max(dk_values)}\n"
    stats += f"Final dk: {dk_values[-1]}"
    ax1.text(0.02, 0.98, stats, transform=ax1.transAxes, fontsize=9,
             verticalalignment='top', bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.7))
    
    # Plot 2: Info box about large graphs
    ax2 = axes[1]
    ax2.text(0.5, 0.5, f'dk(G) Statistics:\n\n'
             f'Graph has n={n} nodes\n'
             f'Only dk approximation computed\n\n'
             f'For exact αk, use graphs with n≤15',
             ha='center', va='center', transform=ax2.transAxes, fontsize=11,
             bbox=dict(boxstyle='round', facecolor='lightblue', alpha=0.7))
    ax2.set_title('Note about large graphs', fontsize=12, fontweight='bold')
    ax2.axis('off')
    
    plt.tight_layout()
    
    filename = f"{graph_name.replace(' ', '_')}_dk_only.png"
    plt.savefig(filename, dpi=150, bbox_inches='tight')
    print(f"\n💾 Saved plot to: {filename}")
    plt.close()


def analyze_graph_complete(G, graph_name, max_k=None):
    """
    Complete analysis of a graph with detailed output.
    
    Args:
        G: NetworkX graph
        graph_name: Name of the graph for display
        max_k: Maximum k value to analyze (default: n-1)
    """
    n = G.number_of_nodes()
    m = G.number_of_edges()
    
    print(f"\n{'='*80}")
    print(f"ANALYZING: {graph_name}")
    print(f"{'='*80}")
    
    # Basic properties
    print(f"\n📊 GRAPH PROPERTIES:")
    print(f"   Nodes (n):           {n}")
    print(f"   Edges (m):           {m}")
    print(f"   Average degree:      {2*m/n:.2f}")
    print(f"   Density:             {nx.density(G):.4f}")
    print(f"   Connected:           {'Yes' if nx.is_connected(G) else 'No'}")
    
    if n > 15:
        print(f"\n⚠️  Graph is too large (n={n}) for exact αk(G) computation.")
        print(f"   Only dk(G) will be computed (approximation).")
        # For large graphs, compute only dk
        return analyze_graph_dk_only(G, graph_name, max_k)
    
    # Initialize
    lsa = LargeSetArboricity(G)
    
    # Determine k range
    if max_k is None:
        max_k = min(n - 1, 20)
    else:
        max_k = min(max_k, n - 1)
    
    # Compute for all k values
    print(f"\n🔬 COMPUTING dk(G) AND αk(G) FOR k = 0 TO {max_k}...")
    
    results = []
    k_values = []
    dk_values = []
    alpha_k_values = []
    ratios = []
    
    for k in range(max_k + 1):
        # Compute dk(G)
        dk_G, removal_seq = lsa.modified_degeneracy_algorithm(k)
        
        # Compute exact αk(G)
        alpha_k, best_subgraph = lsa.compute_alpha_k_exact(k)
        
        # Calculate ratio
        ratio = alpha_k / dk_G if dk_G > 0 else float('inf')
        
        # Store results
        k_values.append(k)
        dk_values.append(dk_G)
        alpha_k_values.append(alpha_k)
        ratios.append(ratio)
        
        results.append({
            'k': k,
            'dk(G)': dk_G,
            'αk(G)': alpha_k,
            'Ratio': f"{ratio:.3f}",
            'Lower Bound': '✓' if dk_G <= alpha_k else '✗',
            'Upper Bound': '✓' if alpha_k <= 2 * dk_G else '✗'
        })
    
    # Print results table
    print(f"\n📋 RESULTS TABLE:")
    print()
    headers = ['k', 'dk(G)', 'αk(G)', 'Ratio (α/d)', 'dk≤α', 'α≤2d']
    table_data = [[r['k'], r['dk(G)'], r['αk(G)'], r['Ratio'], 
                   r['Lower Bound'], r['Upper Bound']] for r in results]
    print(tabulate(table_data, headers=headers, tablefmt='grid'))
    
    # Summary statistics
    print(f"\n📈 SUMMARY STATISTICS:")
    print(f"   Minimum ratio:       {min(ratios):.3f}")
    print(f"   Maximum ratio:       {max(ratios):.3f}")
    print(f"   Average ratio:       {np.mean(ratios):.3f}")
    print(f"   Median ratio:        {np.median(ratios):.3f}")
    
    # Check approximation quality
    all_lower_ok = all(dk_values[i] <= alpha_k_values[i] for i in range(len(k_values)))
    all_upper_ok = all(alpha_k_values[i] <= 2 * dk_values[i] for i in range(len(k_values)))
    
    print(f"\n✅ APPROXIMATION VERIFICATION:")
    print(f"   All lower bounds hold (dk ≤ α):  {'✓ YES' if all_lower_ok else '✗ NO'}")
    print(f"   All upper bounds hold (α ≤ 2d):  {'✓ YES' if all_upper_ok else '✗ NO'}")
    
    if max(ratios) == 2.0:
        print(f"   ⚠️  WORST CASE ACHIEVED: Ratio = 2.0")
    elif max(ratios) == 1.0:
        print(f"   🎯 OPTIMAL: All ratios = 1.0 (perfect approximation)")
    
    # Return data for plotting
    return {
        'k_values': k_values,
        'dk_values': dk_values,
        'alpha_k_values': alpha_k_values,
        'ratios': ratios,
        'graph_name': graph_name
    }


def create_correlation_plots(data_dict):
    """
    Create comprehensive correlation plots.
    
    Args:
        data_dict: Dictionary with k_values, dk_values, alpha_k_values, ratios
    """
    k_values = data_dict['k_values']
    dk_values = data_dict['dk_values']
    alpha_k_values = data_dict['alpha_k_values']
    ratios = data_dict['ratios']
    graph_name = data_dict['graph_name']
    
    # Create figure with 2x2 subplots
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle(f'Large-Set-Arboricity Analysis: {graph_name}', 
                 fontsize=16, fontweight='bold')
    
    # Plot 1: dk(G) and αk(G) vs k
    ax1 = axes[0, 0]
    ax1.plot(k_values, dk_values, 'b-o', label='dk(G)', linewidth=2, markersize=6)
    ax1.plot(k_values, alpha_k_values, 'r-s', label='αk(G)', linewidth=2, markersize=6)
    ax1.fill_between(k_values, dk_values, [2*d for d in dk_values], 
                     alpha=0.2, color='green', label='2-approx bounds')
    ax1.set_xlabel('k (parameter)', fontsize=11)
    ax1.set_ylabel('Value', fontsize=11)
    ax1.set_title('dk(G) and αk(G) vs k', fontsize=12, fontweight='bold')
    ax1.legend(fontsize=9)
    ax1.grid(True, alpha=0.3)
    
    # Plot 2: Approximation Ratio vs k
    ax2 = axes[0, 1]
    ax2.plot(k_values, ratios, 'g-^', linewidth=2, markersize=6, label='αk(G)/dk(G)')
    ax2.axhline(y=1.0, color='blue', linestyle='--', alpha=0.5, label='Perfect (ratio=1)')
    ax2.axhline(y=2.0, color='red', linestyle='--', alpha=0.5, label='Worst case (ratio=2)')
    ax2.fill_between(k_values, 1.0, 2.0, alpha=0.1, color='gray', label='Valid range')
    ax2.set_xlabel('k (parameter)', fontsize=11)
    ax2.set_ylabel('Approximation Ratio (αk/dk)', fontsize=11)
    ax2.set_title('Approximation Quality vs k', fontsize=12, fontweight='bold')
    ax2.legend(fontsize=9)
    ax2.grid(True, alpha=0.3)
    ax2.set_ylim([0.9, 2.1])
    
    # Plot 3: Scatter plot - dk(G) vs αk(G)
    ax3 = axes[1, 0]
    ax3.scatter(dk_values, alpha_k_values, c=k_values, cmap='viridis', 
                s=100, alpha=0.7, edgecolors='black')
    
    # Add diagonal lines
    max_val = max(max(dk_values), max(alpha_k_values))
    ax3.plot([0, max_val], [0, max_val], 'b--', alpha=0.5, label='αk = dk (perfect)')
    ax3.plot([0, max_val], [0, 2*max_val], 'r--', alpha=0.5, label='αk = 2dk (worst)')
    
    # Colorbar
    scatter = ax3.scatter(dk_values, alpha_k_values, c=k_values, 
                         cmap='viridis', s=100, alpha=0.7, edgecolors='black')
    plt.colorbar(scatter, ax=ax3, label='k value')
    
    ax3.set_xlabel('dk(G)', fontsize=11)
    ax3.set_ylabel('αk(G)', fontsize=11)
    ax3.set_title('Correlation: dk(G) vs αk(G)', fontsize=12, fontweight='bold')
    ax3.legend(fontsize=9)
    ax3.grid(True, alpha=0.3)
    ax3.set_xlim([0, max_val * 1.1])
    ax3.set_ylim([0, max_val * 1.1])
    
    # Plot 4: Histogram of ratios
    ax4 = axes[1, 1]
    ax4.hist(ratios, bins=min(10, len(set(ratios))), 
             color='purple', alpha=0.7, edgecolor='black')
    ax4.axvline(x=np.mean(ratios), color='red', linestyle='--', 
                linewidth=2, label=f'Mean: {np.mean(ratios):.3f}')
    ax4.axvline(x=np.median(ratios), color='blue', linestyle='--', 
                linewidth=2, label=f'Median: {np.median(ratios):.3f}')
    ax4.set_xlabel('Approximation Ratio (αk/dk)', fontsize=11)
    ax4.set_ylabel('Frequency', fontsize=11)
    ax4.set_title('Distribution of Approximation Ratios', fontsize=12, fontweight='bold')
    ax4.legend(fontsize=9)
    ax4.grid(True, alpha=0.3, axis='y')
    
    plt.tight_layout()
    
    # Save figure
    filename = f"{graph_name.replace(' ', '_')}_analysis.png"
    plt.savefig(filename, dpi=300, bbox_inches='tight')
    print(f"\n💾 Saved plot to: {filename}")
    
    plt.show()
    
    return filename


def main():
    """Main program."""
    print("="*80)
    print("LARGE-SET-ARBORICITY ANALYSIS")
    print("Main Program: Analyze Small SNAP Graphs")
    print("="*80)
    
    # List of small SNAP graphs suitable for exact computation
    small_graphs = {
        'ca-GrQc': 'Collaboration network (General Relativity)',
    }
    '''
        'wiki-Vote': 'Wikipedia voting network',
        'p2p-Gnutella08': 'P2P network (Gnutella)',
    '''
    print("\n📚 Available small SNAP graphs for analysis:")
    for i, (name, desc) in enumerate(small_graphs.items(), 1):
        print(f"   {i}. {name} - {desc}")
    
    print("\n" + "="*80)
    
    # Analyze each graph
    results_all = []
    
    for graph_name, description in small_graphs.items():
        try:
            print(f"\n\n🔄 Loading {graph_name}...")
            
            # Load graph
            G = load_snap_graph(graph_name)
            
            # Check size
            n = G.number_of_nodes()
            '''
            if n > 15:
                print(f"⚠️  Skipping {graph_name} (n={n} > 15, too large for exact computation)")
                continue
            '''
            # Analyze
            data = analyze_graph_complete(G, graph_name, max_k=n-1)
            
            if data is not None:
                results_all.append(data)
                
                # Create plots based on type
                if data.get('dk_only', False):
                    print(f"✓ dk-only analysis complete for {graph_name}")
                else:
                    # Create correlation plots (only for small graphs with exact αk)
                    print(f"\n📊 Creating correlation plots for {graph_name}...")
                    create_correlation_plots(data)
            
        except Exception as e:
            print(f"\n❌ Error analyzing {graph_name}: {e}")
            import traceback
            traceback.print_exc()
    
    # Summary across all graphs
    if results_all:
        print("\n" + "="*80)
        print("SUMMARY ACROSS ALL GRAPHS")
        print("="*80)
        
        for data in results_all:
            name = data['graph_name']
            if data.get('dk_only', False):
                dk_vals = data['dk_values']
                print(f"\n{name} (dk only):")
                print(f"   Min dk:  {min(dk_vals)}")
                print(f"   Max dk:  {max(dk_vals)}")
            else:
                ratios = data['ratios']
                print(f"\n{name}:")
                print(f"   Average ratio: {np.mean(ratios):.3f}")
                print(f"   Max ratio:     {max(ratios):.3f}")
                print(f"   Min ratio:     {min(ratios):.3f}")
    
    print("\n" + "="*80)
    print("ANALYSIS COMPLETE! ✅")
    print("="*80)
    print("\nGenerated files:")
    for data in results_all:
        print(f"   - {data['graph_name'].replace(' ', '_')}_analysis.png")


if __name__ == "__main__":
    # Check if tabulate is installed
    try:
        import tabulate
    except ImportError:
        print("Installing tabulate for better table formatting...")
        import subprocess
        subprocess.check_call([sys.executable, "-m", "pip", "install", "tabulate"])
        import tabulate
    
    main()
