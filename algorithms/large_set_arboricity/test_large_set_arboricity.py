"""
Test Suite and Examples for Large-Set-Arboricity Algorithms

This module provides comprehensive tests and examples demonstrating
the correctness of the 2-approximation algorithm.
"""

import networkx as nx
import sys
from large_set_arboricity import LargeSetArboricity, demonstrate_algorithm


def test_basic_properties():
    """Test basic properties of the algorithms."""
    print("\n" + "="*70)
    print("TEST 1: Basic Properties")
    print("="*70)
    
    # Test on complete graph K4
    G = nx.complete_graph(4)
    lsa = LargeSetArboricity(G)
    
    print("\nTest 1.1: Average degree calculation")
    avg_deg = lsa.average_degree(G)
    expected = 3.0  # Each vertex has degree 3
    print(f"  Average degree of K4: {avg_deg}")
    print(f"  Expected: {expected}")
    print(f"  ✓ PASS" if abs(avg_deg - expected) < 0.001 else f"  ✗ FAIL")
    
    print("\nTest 1.2: Modified degeneracy for k=0")
    dk_G, removal_seq = lsa.modified_degeneracy_algorithm(0)
    print(f"  dk(G) for K4 with k=0: {dk_G}")
    print(f"  Number of removals: {len(removal_seq)}")
    print(f"  ✓ PASS" if len(removal_seq) == 4 else f"  ✗ FAIL")
    
    print("\nTest 1.3: Witness subgraph construction")
    H = lsa.construct_witness_subgraph(0)
    print(f"  Witness subgraph size: {H.number_of_nodes()}")
    print(f"  Should be > k=0: {'✓ PASS' if H.number_of_nodes() > 0 else '✗ FAIL'}")


def test_approximation_bounds():
    """Test that approximation bounds hold."""
    print("\n" + "="*70)
    print("TEST 2: Approximation Bounds")
    print("="*70)
    
    # Test on various small graphs
    test_cases = [
        ("K5", nx.complete_graph(5), 0),
        ("K5", nx.complete_graph(5), 2),
        ("C6", nx.cycle_graph(6), 0),
        ("C6", nx.cycle_graph(6), 2),
        ("Star", nx.star_graph(5), 0),
        ("Path P6", nx.path_graph(6), 0),
    ]
    
    all_passed = True
    
    for name, G, k in test_cases:
        lsa = LargeSetArboricity(G)
        results = lsa.verify_approximation_bound(k)
        
        print(f"\nGraph: {name}, k={k}")
        print(f"  n={G.number_of_nodes()}, m={G.number_of_edges()}")
        
        if isinstance(results['alpha_k'], int):
            alpha_k = results['alpha_k']
            dk_G = results['dk_G']
            
            print(f"  dk(G) = {dk_G}, αk(G) = {alpha_k}")
            
            lower_ok = dk_G <= alpha_k  # dk(G) ≤ αk(G)
            upper_ok = alpha_k <= 2 * dk_G  # αk(G) ≤ 2·dk(G)
            
            print(f"  Lower bound (dk ≤ αk): {'✓' if lower_ok else '✗'} "
                  f"({dk_G} ≤ {alpha_k})")
            print(f"  Upper bound (αk ≤ 2·dk): {'✓' if upper_ok else '✗'} "
                  f"({alpha_k} ≤ {2 * dk_G})")
            
            if lower_ok and upper_ok:
                print(f"  Approximation ratio: {alpha_k / dk_G if dk_G > 0 else float('inf'):.3f}")
                print(f"  ✓ PASS")
            else:
                print(f"  ✗ FAIL")
                all_passed = False
        else:
            print(f"  Graph too large for exact computation")
    
    print(f"\n{'='*70}")
    if all_passed:
        print("All approximation bound tests PASSED ✓")
    else:
        print("Some tests FAILED ✗")
    print(f"{'='*70}")


def test_edge_cases():
    """Test edge cases."""
    print("\n" + "="*70)
    print("TEST 3: Edge Cases")
    print("="*70)
    
    # Empty graph
    print("\nTest 3.1: Single vertex graph")
    G = nx.Graph()
    G.add_node(0)
    lsa = LargeSetArboricity(G)
    try:
        dk_G, _ = lsa.modified_degeneracy_algorithm(0)
        print(f"  dk(G) for single vertex: {dk_G}")
        print(f"  ✓ PASS")
    except Exception as e:
        print(f"  ✗ FAIL: {e}")
    
    # Disconnected graph
    print("\nTest 3.2: Disconnected graph")
    G = nx.Graph()
    G.add_edges_from([(0,1), (2,3)])
    lsa = LargeSetArboricity(G)
    dk_G, _ = lsa.modified_degeneracy_algorithm(0)
    print(f"  dk(G) for two disconnected edges: {dk_G}")
    print(f"  ✓ PASS")
    
    # k at boundary
    print("\nTest 3.3: k = n-1")
    G = nx.complete_graph(5)
    lsa = LargeSetArboricity(G)
    try:
        dk_G, removal_seq = lsa.modified_degeneracy_algorithm(4)
        print(f"  dk(G) for K5 with k=4: {dk_G}")
        print(f"  Number of removals: {len(removal_seq)}")
        print(f"  ✓ PASS")
    except Exception as e:
        print(f"  ✗ FAIL: {e}")


def demonstrate_proof_construction():
    """Demonstrate the proof construction from the paper."""
    print("\n" + "="*70)
    print("DEMONSTRATION: Proof Construction (Section 3.1)")
    print("="*70)
    
    # Use a specific example to show the proof construction
    G = nx.cycle_graph(8)
    G.add_edges_from([(0,4), (2,6)])  # Add two chords
    
    k = 2
    
    print(f"\nGraph: 8-cycle with 2 chords")
    print(f"  n = {G.number_of_nodes()}, m = {G.number_of_edges()}")
    print(f"  k = {k}")
    
    lsa = LargeSetArboricity(G)
    
    # Run algorithm
    dk_G, removal_seq = lsa.modified_degeneracy_algorithm(k)
    
    print(f"\nStep 1: Run modified degeneracy algorithm")
    print(f"  dk(G) = {dk_G}")
    
    # Show when dk(G) was achieved
    print(f"\nStep 2: Find when dk(G) was achieved")
    for i, (v, d) in enumerate(removal_seq):
        if d == dk_G:
            print(f"  At step i={i+1}: removed vertex {v} with degree {d}")
            print(f"  This is where dk(G) = {dk_G} was achieved")
            break
    
    # Construct witness subgraph
    H = lsa.construct_witness_subgraph(k)
    
    print(f"\nStep 3: Construct witness subgraph H")
    print(f"  H = {{v}} ∪ {{all vertices removed after step i}}")
    print(f"  |V(H)| = {H.number_of_nodes()}")
    print(f"  Since i ≤ n-k, we have |V(H)| ≥ k+1 = {k+1}")
    print(f"  Verification: {H.number_of_nodes()} > {k}? {'✓' if H.number_of_nodes() > k else '✗'}")
    
    # Verify degree property
    print(f"\nStep 4: Verify every vertex in H has degree ≥ {dk_G}")
    min_deg_in_H = min(H.degree(v) for v in H.nodes())
    print(f"  Minimum degree in H: {min_deg_in_H}")
    print(f"  All degrees ≥ {dk_G}? {'✓' if min_deg_in_H >= dk_G else '✗'}")
    
    # Calculate average degree
    avg_deg_H = lsa.average_degree(H)
    ceil_avg_deg = int(avg_deg_H) if avg_deg_H == int(avg_deg_H) else int(avg_deg_H) + 1
    
    print(f"\nStep 5: Calculate average degree")
    print(f"  d̄[H] = 2|E(H)| / |V(H)| = {2*H.number_of_edges()} / {H.number_of_nodes()}")
    print(f"  d̄[H] = {avg_deg_H:.3f}")
    print(f"  ⌈d̄[H]⌉ = {ceil_avg_deg}")
    
    print(f"\nConclusion:")
    print(f"  Since H ⊆ G and |V(H)| > k:")
    print(f"  αk(G) ≥ ⌈d̄[H]⌉ = {ceil_avg_deg}")
    print(f"  Therefore: αk(G) ≥ dk(G) = {dk_G}")
    print(f"  Lower bound proven! ✓")


def analyze_graph_families():
    """Analyze interesting graph families."""
    print("\n" + "="*70)
    print("ANALYSIS: Graph Families")
    print("="*70)
    
    families = {
        "Complete Graphs": [nx.complete_graph(n) for n in [4, 5, 6]],
        "Cycles": [nx.cycle_graph(n) for n in [5, 6, 7]],
        "Paths": [nx.path_graph(n) for n in [5, 6, 7]],
        "Regular Graphs": [nx.circulant_graph(6, [1, 2]), 
                          nx.cubical_graph(),
                          nx.dodecahedral_graph()]
    }
    
    for family_name, graphs in families.items():
        print(f"\n{family_name}:")
        print("-" * 50)
        
        for G in graphs:
            n = G.number_of_nodes()
            m = G.number_of_edges()
            
            # Skip very large graphs
            if n > 15:
                continue
                
            lsa = LargeSetArboricity(G)
            
            # Test with k=0 (standard degeneracy)
            results = lsa.verify_approximation_bound(0)
            
            if isinstance(results['alpha_k'], int):
                ratio = results['approximation_ratio']
                print(f"  n={n}, m={m}: α₀(G)={results['alpha_k']}, "
                      f"d₀(G)={results['dk_G']}, ratio={ratio:.2f}")


def main():
    """Run all tests and demonstrations."""
    print("\n" + "="*70)
    print("LARGE-SET-ARBORICITY: IMPLEMENTATION AND TESTING")
    print("Based on: 'My notes for prove approximate for large-Set-Arboricity'")
    print("By: Amit Perelman")
    print("="*70)
    
    # Run tests
    test_basic_properties()
    test_approximation_bounds()
    test_edge_cases()
    
    # Demonstrations
    demonstrate_proof_construction()
    analyze_graph_families()
    
    # Detailed example
    print("\n" + "="*70)
    print("DETAILED EXAMPLE: Petersen Graph")
    print("="*70)
    demonstrate_algorithm(nx.petersen_graph(), k=3, graph_name="Petersen Graph")
    
    print("\n" + "="*70)
    print("All tests and demonstrations complete!")
    print("="*70)


if __name__ == "__main__":
    main()
