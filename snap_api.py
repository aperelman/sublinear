#!/usr/bin/env python3
"""
SNAP Graph Loader - Enhanced Version
Downloads real graphs from Stanford SNAP dataset
Works on Windows/Fedora with caching support
"""

import networkx as nx
import urllib.request
import gzip
import io
import os
from typing import Optional, Dict


class SNAPLoader:
    """
    SNAP graph dataset loader with multi-graph support.
    
    Features:
    - Downloads from Stanford SNAP
    - Local caching to avoid re-downloads
    - Support for 20+ popular SNAP datasets
    - Automatic graph preprocessing
    """
    
    # Dataset URLs from SNAP
    DATASETS = {
        # Collaboration Networks
        'ca-GrQc': 'https://snap.stanford.edu/data/ca-GrQc.txt.gz',
        'ca-HepTh': 'https://snap.stanford.edu/data/ca-HepTh.txt.gz',
        'ca-HepPh': 'https://snap.stanford.edu/data/ca-HepPh.txt.gz',
        'ca-AstroPh': 'https://snap.stanford.edu/data/ca-AstroPh.txt.gz',
        'ca-CondMat': 'https://snap.stanford.edu/data/ca-CondMat.txt.gz',
        
        # Social Networks
        'ego-Facebook': 'https://snap.stanford.edu/data/facebook_combined.txt.gz',
        'ego-Gplus': 'https://snap.stanford.edu/data/gplus_combined.txt.gz',
        'ego-Twitter': 'https://snap.stanford.edu/data/twitter_combined.txt.gz',
        'soc-Epinions1': 'https://snap.stanford.edu/data/soc-Epinions1.txt.gz',
        'soc-Slashdot0811': 'https://snap.stanford.edu/data/soc-Slashdot0811.txt.gz',
        'soc-Slashdot0902': 'https://snap.stanford.edu/data/soc-Slashdot0902.txt.gz',
        
        # Communication Networks
        'email-Enron': 'https://snap.stanford.edu/data/email-Enron.txt.gz',
        'email-EuAll': 'https://snap.stanford.edu/data/email-EuAll.txt.gz',
        'wiki-Vote': 'https://snap.stanford.edu/data/wiki-Vote.txt.gz',
        
        # P2P Networks
        'p2p-Gnutella04': 'https://snap.stanford.edu/data/p2p-Gnutella04.txt.gz',
        'p2p-Gnutella08': 'https://snap.stanford.edu/data/p2p-Gnutella08.txt.gz',
        'p2p-Gnutella09': 'https://snap.stanford.edu/data/p2p-Gnutella09.txt.gz',
        
        # Amazon Networks
        'amazon0302': 'https://snap.stanford.edu/data/amazon0302.txt.gz',
        'amazon0312': 'https://snap.stanford.edu/data/amazon0312.txt.gz',
        'amazon0505': 'https://snap.stanford.edu/data/amazon0505.txt.gz',
        'amazon0601': 'https://snap.stanford.edu/data/amazon0601.txt.gz',
    }
    
    # Dataset metadata (approximate sizes)
    METADATA = {
        'ca-GrQc': {'n': 5242, 'm': 14496, 'desc': 'General Relativity collaboration'},
        'ca-HepTh': {'n': 9877, 'm': 25998, 'desc': 'High Energy Physics Theory collaboration'},
        'ca-HepPh': {'n': 12008, 'm': 118521, 'desc': 'High Energy Physics Phenomenology'},
        'ca-AstroPh': {'n': 18772, 'm': 198110, 'desc': 'Astrophysics collaboration'},
        'ca-CondMat': {'n': 23133, 'm': 93497, 'desc': 'Condensed Matter collaboration'},
        'ego-Facebook': {'n': 4039, 'm': 88234, 'desc': 'Facebook social circles'},
        'ego-Gplus': {'n': 107614, 'm': 13673453, 'desc': 'Google+ social circles'},
        'ego-Twitter': {'n': 81306, 'm': 1768149, 'desc': 'Twitter social circles'},
        'soc-Epinions1': {'n': 75879, 'm': 508837, 'desc': 'Epinions trust network'},
        'soc-Slashdot0811': {'n': 77360, 'm': 905468, 'desc': 'Slashdot social network'},
        'soc-Slashdot0902': {'n': 82168, 'm': 948464, 'desc': 'Slashdot social network'},
        'email-Enron': {'n': 36692, 'm': 183831, 'desc': 'Enron email network'},
        'email-EuAll': {'n': 265214, 'm': 420045, 'desc': 'EU email network'},
        'wiki-Vote': {'n': 7115, 'm': 103689, 'desc': 'Wikipedia voting network'},
        'p2p-Gnutella04': {'n': 10876, 'm': 39994, 'desc': 'Gnutella P2P network'},
        'p2p-Gnutella08': {'n': 6301, 'm': 20777, 'desc': 'Gnutella P2P network'},
        'p2p-Gnutella09': {'n': 8114, 'm': 26013, 'desc': 'Gnutella P2P network'},
        'amazon0302': {'n': 262111, 'm': 1234877, 'desc': 'Amazon product network'},
        'amazon0312': {'n': 400727, 'm': 3200440, 'desc': 'Amazon product network'},
        'amazon0505': {'n': 410236, 'm': 3356824, 'desc': 'Amazon product network'},
        'amazon0601': {'n': 403394, 'm': 3387388, 'desc': 'Amazon product network'},
    }
    
    def __init__(self, cache_dir: str = './snap_cache'):
        """
        Initialize SNAP loader.
        
        Args:
            cache_dir: Directory for caching downloaded files
        """
        self.cache_dir = cache_dir
        os.makedirs(cache_dir, exist_ok=True)
    
    def load(self, dataset_name: str, 
             use_cache: bool = True,
             largest_component: bool = True,
             remove_self_loops: bool = True) -> nx.Graph:
        """
        Load a SNAP graph by name.
        
        Args:
            dataset_name: Name of dataset (e.g., 'ca-GrQc')
            use_cache: Use cached file if available
            largest_component: Extract largest connected component
            remove_self_loops: Remove self-loops from graph
            
        Returns:
            NetworkX Graph
        """
        if dataset_name not in self.DATASETS:
            raise ValueError(f"Unknown dataset: {dataset_name}\n"
                           f"Available: {list(self.DATASETS.keys())}")
        
        # Get metadata
        meta = self.METADATA.get(dataset_name, {})
        print(f"Loading {dataset_name}...")
        if 'desc' in meta:
            print(f"  Description: {meta['desc']}")
        if 'n' in meta and 'm' in meta:
            print(f"  Expected: ~{meta['n']:,} nodes, ~{meta['m']:,} edges")
        
        # Download/load
        G = self._download_and_parse(dataset_name, use_cache)
        
        # Preprocessing
        if remove_self_loops:
            n_self_loops = nx.number_of_selfloops(G)
            if n_self_loops > 0:
                G.remove_edges_from(nx.selfloop_edges(G))
                print(f"  Removed {n_self_loops} self-loops")
        
        if largest_component and not nx.is_connected(G):
            components = list(nx.connected_components(G))
            largest = max(components, key=len)
            G = G.subgraph(largest).copy()
            print(f"  Extracted largest component: {len(largest):,} nodes")
        
        n = G.number_of_nodes()
        m = G.number_of_edges()
        print(f"✓ Loaded: {n:,} nodes, {m:,} edges")
        print(f"  Average degree: {2*m/n:.2f}")
        
        return G
    
    def _download_and_parse(self, dataset_name: str, use_cache: bool) -> nx.Graph:
        """Download and parse graph from SNAP."""
        url = self.DATASETS[dataset_name]
        cache_file = os.path.join(self.cache_dir, f'{dataset_name}.txt.gz')
        
        # Check cache
        if use_cache and os.path.exists(cache_file):
            print(f"  Using cached file: {cache_file}")
            with gzip.open(cache_file, 'rt') as f:
                return self._parse_snap_edgelist(f.read())
        
        # Download
        print(f"  Downloading from SNAP...")
        try:
            with urllib.request.urlopen(url) as response:
                compressed_data = response.read()
            
            # Save to cache
            with open(cache_file, 'wb') as f:
                f.write(compressed_data)
            print(f"  ✓ Downloaded and cached to {cache_file}")
            
            # Parse
            with gzip.GzipFile(fileobj=io.BytesIO(compressed_data)) as f:
                content = f.read().decode('utf-8')
            
            return self._parse_snap_edgelist(content)
        
        except Exception as e:
            print(f"✗ Error downloading: {e}")
            print("\nTroubleshooting:")
            print("1. Check internet connection")
            print(f"2. Try: wget {url}")
            print(f"3. Place file in: {self.cache_dir}/")
            raise
    
    def _parse_snap_edgelist(self, text_content: str) -> nx.Graph:
        """Parse SNAP edge list format (lines with comments starting with #)."""
        G = nx.Graph()
        
        for line in text_content.split('\n'):
            line = line.strip()
            # Skip comments and empty lines
            if not line or line.startswith('#'):
                continue
            
            parts = line.split()
            if len(parts) >= 2:
                try:
                    u, v = int(parts[0]), int(parts[1])
                    G.add_edge(u, v)
                except ValueError:
                    continue  # Skip malformed lines
        
        return G
    
    @classmethod
    def list_datasets(cls) -> None:
        """Print all available datasets."""
        print("\nAvailable SNAP datasets:")
        print("-" * 80)
        
        categories = {
            'Collaboration Networks': ['ca-GrQc', 'ca-HepTh', 'ca-HepPh', 'ca-AstroPh', 'ca-CondMat'],
            'Social Networks': ['ego-Facebook', 'ego-Gplus', 'ego-Twitter', 'soc-Epinions1', 
                               'soc-Slashdot0811', 'soc-Slashdot0902'],
            'Communication Networks': ['email-Enron', 'email-EuAll', 'wiki-Vote'],
            'P2P Networks': ['p2p-Gnutella04', 'p2p-Gnutella08', 'p2p-Gnutella09'],
            'Product Networks': ['amazon0302', 'amazon0312', 'amazon0505', 'amazon0601'],
        }
        
        for category, datasets in categories.items():
            print(f"\n{category}:")
            for name in datasets:
                if name in cls.METADATA:
                    meta = cls.METADATA[name]
                    print(f"  {name:<20} n~{meta['n']:>7,}  m~{meta['m']:>9,}  {meta['desc']}")
                else:
                    print(f"  {name}")
        
        print("\n" + "-" * 80)
        print(f"Total: {len(cls.DATASETS)} datasets available")
        print()


# Convenience functions
def load_snap_graph(dataset_name: str, cache_dir: str = './snap_cache', **kwargs) -> nx.Graph:
    """
    Convenience function to load a SNAP graph.
    
    Args:
        dataset_name: Name of dataset (e.g., 'ca-GrQc')
        cache_dir: Directory for caching
        **kwargs: Additional arguments for SNAPLoader.load()
        
    Returns:
        NetworkX Graph
    """
    loader = SNAPLoader(cache_dir=cache_dir)
    return loader.load(dataset_name, **kwargs)


def list_snap_datasets() -> None:
    """List all available SNAP datasets."""
    SNAPLoader.list_datasets()


# Test/demo script
if __name__ == '__main__':
    import sys
    
    # Show available datasets
    if len(sys.argv) == 1:
        print("="*80)
        print("SNAP Graph Loader - Enhanced Version")
        print("="*80)
        list_snap_datasets()
        
        print("\nUsage:")
        print("  python snap_api.py <dataset_name>")
        print("\nExamples:")
        print("  python snap_api.py ca-GrQc")
        print("  python snap_api.py ego-Facebook")
        print("  python snap_api.py email-Enron")
        sys.exit(0)
    
    # Load specific dataset
    dataset_name = sys.argv[1]
    
    print("="*80)
    print(f"Testing SNAP Loader with {dataset_name}")
    print("="*80)
    print()
    
    try:
        G = load_snap_graph(dataset_name)
        
        print("\n" + "="*80)
        print("GRAPH STATISTICS")
        print("="*80)
        print(f"Nodes: {G.number_of_nodes():,}")
        print(f"Edges: {G.number_of_edges():,}")
        print(f"Average degree: {2*G.number_of_edges()/G.number_of_nodes():.2f}")
        print(f"Density: {nx.density(G):.6f}")
        print(f"Connected: {nx.is_connected(G)}")
        
        if G.number_of_nodes() <= 10000:
            print(f"Clustering coefficient: {nx.average_clustering(G):.4f}")
        
        print("\n✓ Graph loaded successfully!")
        
    except Exception as e:
        print(f"\n✗ Error: {e}")
        sys.exit(1)
