#!/usr/bin/env python3
"""
Plotting utilities for large-set-arboricity analysis
"""

import matplotlib.pyplot as plt
import numpy as np
from typing import List, Tuple


def compute_alpha_k_for_all_k(lsa, max_k=None):
    """
    Compute dk and αk for all k values from 1 to max_k
    
    Args:
        lsa: LargeSetArboricity instance
        max_k: Maximum k value (default: n-1)
    
    Returns:
        (k_values, dk_values, alpha_k_values)
    """
    n = lsa.n
    if max_k is None or max_k > n - 1:
        max_k = n - 1
    
    k_values = []
    dk_values = []
    alpha_k_values = []
    
    for k in range(1, max_k + 1):
        dk, _ = lsa.modified_degeneracy_algorithm(k)
        alpha_k, _ = lsa.compute_alpha_k_exact(k)
        
        k_values.append(k)
        dk_values.append(dk)
        alpha_k_values.append(alpha_k if alpha_k is not None else dk)
    
    return k_values, dk_values, alpha_k_values


def plot_alpha_k_vs_k(k_values, dk_values, alpha_k_values, graph_name="Graph", save_path=None):
    """
    Create a plot showing dk(G) and αk(G) vs k
    
    Args:
        k_values: List of k values
        dk_values: List of dk(G) values
        alpha_k_values: List of αk(G) values
        graph_name: Name for the plot title
        save_path: Optional path to save the plot
    """
    fig, ax = plt.subplots(figsize=(10, 6))
    
    # Plot dk and αk
    ax.plot(k_values, dk_values, 'b-o', label='dk(G)', linewidth=2, markersize=6)
    ax.plot(k_values, alpha_k_values, 'r-s', label='αk(G)', linewidth=2, markersize=6)
    
    # Fill approximation bounds
    ax.fill_between(k_values, dk_values, [2*d for d in dk_values], 
                     alpha=0.2, color='green', label='2-approximation bounds')
    
    # Labels and title
    ax.set_xlabel('k (parameter)', fontsize=12)
    ax.set_ylabel('Value', fontsize=12)
    ax.set_title(f'Large-Set-Arboricity Analysis: {graph_name}', 
                 fontsize=14, fontweight='bold')
    ax.legend(fontsize=10)
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    
    if save_path:
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        print(f"Saved plot to: {save_path}")
    
    return fig


def plot_approximation_quality(k_values, dk_values, alpha_k_values, graph_name="Graph", save_path=None):
    """
    Plot approximation quality (ratio αk/dk)
    
    Args:
        k_values: List of k values
        dk_values: List of dk(G) values  
        alpha_k_values: List of αk(G) values
        graph_name: Name for the plot title
        save_path: Optional path to save the plot
    """
    ratios = [a/d if d > 0 else 0 for a, d in zip(alpha_k_values, dk_values)]
    
    fig, ax = plt.subplots(figsize=(10, 6))
    
    ax.plot(k_values, ratios, 'g-^', linewidth=2, markersize=6, label='αk(G)/dk(G)')
    ax.axhline(y=1.0, color='blue', linestyle='--', alpha=0.5, label='Perfect (ratio=1)')
    ax.axhline(y=2.0, color='red', linestyle='--', alpha=0.5, label='Worst case (ratio=2)')
    ax.fill_between(k_values, 1.0, 2.0, alpha=0.1, color='gray', label='Valid range')
    
    ax.set_xlabel('k (parameter)', fontsize=12)
    ax.set_ylabel('Approximation Ratio (αk/dk)', fontsize=12)
    ax.set_title(f'Approximation Quality: {graph_name}', fontsize=14, fontweight='bold')
    ax.set_ylim([0.9, 2.1])
    ax.legend(fontsize=10)
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    
    if save_path:
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
        print(f"Saved plot to: {save_path}")
    
    return fig


if __name__ == '__main__':
    # Test plotting
    print("Testing plot functions...")
    k_values = [1, 2, 3, 4, 5]
    dk_values = [2, 3, 3, 4, 4]
    alpha_k_values = [2, 4, 5, 6, 7]
    
    plot_alpha_k_vs_k(k_values, dk_values, alpha_k_values, "Test Graph")
    plt.show()
