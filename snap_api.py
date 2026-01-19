#!/usr/bin/env python3
"""
SNAP Graph Loader - Downloads real ca-GrQc from SNAP
Works on your local machine (Windows/Fedora)
"""

import networkx as nx
import urllib.request
import gzip
import io
import os


class SNAPLoader:
    """SNAP graph dataset loader"""
    
    def __init__(self, cache_dir='./snap_data'):
        """Initialize with cache directory for downloaded graphs"""
        self.cache_dir = cache_dir
        os.makedirs(cache_dir, exist_ok=True)
    
    def download_ca_grqc(self):
        """
        Download real ca-GrQc from SNAP
        Real stats: n=5,242 nodes, m=14,496 edges
        """
        cache_file = os.path.join(self.cache_dir, 'ca-GrQc.txt.gz')
        
        # Check if already cached
        if os.path.exists(cache_file):
            print(f"Using cached file: {cache_file}")
            with gzip.open(cache_file, 'rt') as f:
                return self._parse_snap_edgelist(f.read())
        
        # Download from SNAP
        url = "https://snap.stanford.edu/data/ca-GrQc.txt.gz"
        print(f"Downloading ca-GrQc from SNAP...")
        print(f"URL: {url}")
        print(f"Saving to: {cache_file}")
        
        try:
            with urllib.request.urlopen(url) as response:
                compressed_data = response.read()
            
            # Save to cache
            with open(cache_file, 'wb') as f:
                f.write(compressed_data)
            print(f"✓ Downloaded and cached")
            
            # Parse
            with gzip.GzipFile(fileobj=io.BytesIO(compressed_data)) as f:
                content = f.read().decode('utf-8')
            
            return self._parse_snap_edgelist(content)
        
        except Exception as e:
            print(f"Error downloading: {e}")
            print("\nTroubleshooting:")
            print("1. Check internet connection")
            print("2. Try: wget https://snap.stanford.edu/data/ca-GrQc.txt.gz")
            print(f"3. Place file in: {self.cache_dir}/")
            raise
    
    def _parse_snap_edgelist(self, text_content):
        """Parse SNAP edge list format"""
        G = nx.Graph()
        
        for line in text_content.split('\n'):
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            
            parts = line.split()
            if len(parts) >= 2:
                try:
                    u, v = int(parts[0]), int(parts[1])
                    if u != v:  # No self-loops
                        G.add_edge(u, v)
                except ValueError:
                    continue
        
        return G


def load_snap_graph(graph_name):
    """
    Load SNAP graph by name
    
    Args:
        graph_name: 'ca-GrQc' or other SNAP dataset
        
    Returns:
        NetworkX Graph
    """
    loader = SNAPLoader()
    
    if graph_name.lower() in ['ca-grqc', 'ca-GrQc']:
        print(f"Loading ca-GrQc (General Relativity collaboration network)...")
        G = loader.download_ca_grqc()
        n = G.number_of_nodes()
        m = G.number_of_edges()
        print(f"✓ Loaded: {n:,} nodes, {m:,} edges")
        
        # Verify it's the real thing
        if n > 5000:
            print(f"✓ This is the REAL ca-GrQc from SNAP!")
        else:
            print(f"⚠️  Warning: Expected ~5,242 nodes, got {n}")
        
        return G
    else:
        raise ValueError(f"Unknown graph: {graph_name}\nSupported: ca-GrQc")


# Test function
if __name__ == '__main__':
    print("Testing SNAP loader...")
    G = load_snap_graph('ca-GrQc')
    print(f"\nGraph loaded successfully!")
    print(f"Nodes: {G.number_of_nodes():,}")
    print(f"Edges: {G.number_of_edges():,}")
    print(f"Average degree: {2*G.number_of_edges()/G.number_of_nodes():.2f}")
