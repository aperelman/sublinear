#!/usr/bin/env python3
"""
Simple usage example for Large-Set-Arboricity algorithms
"""

import networkx as nx
from large_set_arboricity import LargeSetArboricity, demonstrate_algorithm

print("="*70)
print("LARGE-SET-ARBORICITY: Quick Start Examples")
print("="*70)

# Example 1: Complete Graph
print("\n\nExample 1: Complete Graph K5")
print("-" * 50)
G = nx.complete_graph(5)
lsa = LargeSetArboricity(G)

k = 0
dk_G, removal_seq = lsa.modified_degeneracy_algorithm(k)
alpha_k, best_subgraph = lsa.compute_alpha_k_exact(k)

print(f"Graph: K5 (complete graph on 5 vertices)")
print(f"dk(G) = {dk_G}")
print(f"αk(G) = {alpha_k}")
print(f"Approximation ratio: {alpha_k / dk_G:.2f}")

# Example 2: Star Graph
print("\n\nExample 2: Star Graph S5")
print("-" * 50)
G = nx.star_graph(5)
lsa = LargeSetArboricity(G)

k = 0
dk_G, removal_seq = lsa.modified_degeneracy_algorithm(k)
alpha_k, best_subgraph = lsa.compute_alpha_k_exact(k)

print(f"Graph: S5 (star with 1 center + 5 leaves)")
print(f"dk(G) = {dk_G}")
print(f"αk(G) = {alpha_k}")
print(f"Approximation ratio: {alpha_k / dk_G:.2f}")
print("Note: This achieves the worst-case ratio of 2.0!")

# Example 3: Cycle
print("\n\nExample 3: Cycle C8")
print("-" * 50)
G = nx.cycle_graph(8)
lsa = LargeSetArboricity(G)

k = 0
dk_G, removal_seq = lsa.modified_degeneracy_algorithm(k)
alpha_k, best_subgraph = lsa.compute_alpha_k_exact(k)

print(f"Graph: C8 (cycle on 8 vertices)")
print(f"dk(G) = {dk_G}")
print(f"αk(G) = {alpha_k}")
print(f"Approximation ratio: {alpha_k / dk_G:.2f}")

# Example 4: Different k values on Petersen graph
print("\n\nExample 4: Petersen Graph with varying k")
print("-" * 50)
G = nx.petersen_graph()

for k in [0, 3, 7]:
    lsa = LargeSetArboricity(G)
    dk_G, _ = lsa.modified_degeneracy_algorithm(k)
    
    if k <= 7:  # Only compute exact for reasonable k
        alpha_k, _ = lsa.compute_alpha_k_exact(k)
        ratio = alpha_k / dk_G if dk_G > 0 else float('inf')
        print(f"k={k}: dk(G)={dk_G}, αk(G)={alpha_k}, ratio={ratio:.2f}")
    else:
        print(f"k={k}: dk(G)={dk_G}")

# Example 5: Detailed analysis
print("\n\nExample 5: Detailed Analysis of Custom Graph")
print("-" * 50)
G = nx.cycle_graph(6)
G.add_edges_from([(0, 3), (1, 4)])  # Add two chords
demonstrate_algorithm(G, k=2, graph_name="6-cycle with 2 chords")

print("\n" + "="*70)
print("Examples complete!")
print("="*70)
print("\nFor more examples, run: python test_large_set_arboricity.py")
print("For full tests, run: python large_set_arboricity.py")
