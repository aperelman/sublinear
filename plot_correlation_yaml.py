#!/usr/bin/env python3
"""
Simple plot: Show correlation between α_k and k
Now with YAML configuration support!

Usage:
    python plot_correlation_yaml.py                    # Uses config.yml
    python plot_correlation_yaml.py custom_config.yml  # Uses custom config
    python plot_correlation_yaml.py --graph ca-GrQc    # Override graph from command line
"""

import sys
import yaml
from pathlib import Path
import matplotlib.pyplot as plt
from snap_api import load_snap_graph
from large_set_arboricity import LargeSetArboricity


def load_config(config_file="config.yml"):
    """Load configuration from YAML file."""
    try:
        with open(config_file, 'r', encoding='utf-8') as f:
            config = yaml.safe_load(f)
        print(f"✅ Loaded configuration from: {config_file}")
        return config
    except FileNotFoundError:
        print(f"⚠️  Config file not found: {config_file}")
        print(f"   Creating default config.yml...")
        create_default_config()
        return load_config(config_file)
    except Exception as e:
        print(f"❌ Error loading config: {e}")
        sys.exit(1)


def create_default_config():
    """Create a default config.yml if it doesn't exist."""
    default_config = {
        'graph': {
            'name': 'test_graph.txt',
            'display_name': '',
            'max_k': None,
            'auto_download': True
        },
        'output': {
            'output_dir': '.',
            'filename': '',
            'format': 'png',
            'dpi': 300,
            'show_plot': False
        },
        'plot': {
            'figure_size': [10, 7],
            'title_fontsize': 15,
            'label_fontsize': 14,
            'legend_fontsize': 12,
            'info_box_fontsize': 10,
            'alpha_k_color': 'red',
            'dk_color': 'blue',
            'alpha_k_linestyle': 'o-',
            'dk_linestyle': 's--',
            'linewidth': 3,
            'markersize': 8,
            'show_grid': True,
            'grid_alpha': 0.3,
            'show_value_labels': True
        },
        'info_box': {
            'show': True,
            'position': 'upper left',
            'constant_color': 'lightblue',
            'varying_color': 'lightgreen'
        },
        'computation': {
            'max_nodes_for_exact': 15,
            'verbose': True,
            'print_table': True
        }
    }
    
    with open('config.yml', 'w', encoding='utf-8') as f:
        yaml.dump(default_config, f, default_flow_style=False, allow_unicode=True)
    
    print("✅ Created default config.yml")


def plot_alpha_k_correlation(config, graph_override=None):
    """
    Plot α_k vs k to show correlation using configuration.
    
    Args:
        config: Configuration dictionary from YAML
        graph_override: Optional graph name to override config
    """
    # Get settings from config
    graph_name = graph_override or config['graph']['name']
    display_name = config['graph'].get('display_name', '')
    max_k_config = config['graph'].get('max_k')
    
    output_dir = config['output'].get('output_dir', '.')
    output_format = config['output'].get('format', 'png')
    dpi = config['output'].get('dpi', 300)
    show_plot = config['output'].get('show_plot', False)
    
    plot_config = config.get('plot', {})
    info_box_config = config.get('info_box', {})
    comp_config = config.get('computation', {})
    
    verbose = comp_config.get('verbose', True)
    print_table = comp_config.get('print_table', True)
    max_nodes_exact = comp_config.get('max_nodes_for_exact', 15)
    
    # Load graph
    if verbose:
        print(f"\n{'='*60}")
        print(f"Loading graph: {graph_name}")
        print(f"{'='*60}")
    
    G = load_snap_graph(graph_name)
    
    n = G.number_of_nodes()
    m = G.number_of_edges()
    avg_degree = 2 * m / n
    
    if verbose:
        print(f"Graph: n={n}, m={m}, avg_degree={avg_degree:.2f}")
    
    # Check if exact computation is feasible
    if n > max_nodes_exact:
        if verbose:
            print(f"⚠️  Graph too large (n={n} > {max_nodes_exact}). Only computing dk(G).")
        compute_exact = False
    else:
        compute_exact = True
    
    # Initialize
    lsa = LargeSetArboricity(G)
    
    # Compute for all k values
    k_values = []
    alpha_k_values = []
    dk_values = []
    
    max_k = max_k_config if max_k_config is not None else min(n - 1, 20)
    
    if verbose:
        print(f"Computing for k = 0 to {max_k}...")
    
    for k in range(max_k + 1):
        dk_G, _ = lsa.modified_degeneracy_algorithm(k)
        dk_values.append(dk_G)
        k_values.append(k)
        
        if compute_exact:
            alpha_k, _ = lsa.compute_alpha_k_exact(k)
            alpha_k_values.append(alpha_k)
        
        if verbose and (k + 1) % 5 == 0:
            print(f"  k={k}")
    
    # Create plot
    fig_size = plot_config.get('figure_size', [10, 7])
    plt.figure(figsize=fig_size)
    
    # Colors and styles
    alpha_color = plot_config.get('alpha_k_color', 'red')
    dk_color = plot_config.get('dk_color', 'blue')
    alpha_style = plot_config.get('alpha_k_linestyle', 'o-')
    dk_style = plot_config.get('dk_linestyle', 's--')
    linewidth = plot_config.get('linewidth', 3)
    markersize = plot_config.get('markersize', 8)
    
    if compute_exact:
        # Plot α_k vs k
        plt.plot(k_values, alpha_k_values, alpha_style, color=alpha_color,
                linewidth=linewidth, markersize=markersize, 
                label='α_k(G) - Exact', alpha=0.8)
        
        # Also show dk for comparison
        plt.plot(k_values, dk_values, dk_style, color=dk_color,
                linewidth=linewidth-1, markersize=markersize-2, 
                label='d_k(G) - Approximation', alpha=0.6)
        
        # Add value labels if requested
        if plot_config.get('show_value_labels', True):
            for k, alpha in zip(k_values, alpha_k_values):
                plt.text(k, alpha + 0.1, f'{alpha}', ha='center', 
                        fontsize=9, color=alpha_color, fontweight='bold')
    else:
        # Only dk available
        plt.plot(k_values, dk_values, dk_style.replace('--', '-'), 
                color=dk_color, linewidth=linewidth, markersize=markersize,
                label='d_k(G) - Approximation')
        
        if plot_config.get('show_value_labels', True):
            for k, dk in zip(k_values, dk_values):
                plt.text(k, dk + 0.1, f'{dk}', ha='center', 
                        fontsize=9, color=dk_color, fontweight='bold')
    
    # Determine display name
    if not display_name:
        display_name = graph_name.replace('.txt', '').replace('_', ' ').title()
    
    # Formatting
    label_fontsize = plot_config.get('label_fontsize', 14)
    title_fontsize = plot_config.get('title_fontsize', 15)
    legend_fontsize = plot_config.get('legend_fontsize', 12)
    
    plt.xlabel('k (parameter)', fontsize=label_fontsize, fontweight='bold')
    plt.ylabel('α_k(G) value', fontsize=label_fontsize, fontweight='bold')
    
    # Enhanced title
    title = f'Graph: {display_name}\n'
    title += f'Correlation between α_k and k\n'
    title += f'n={n} nodes  |  m={m} edges  |  Average Degree={avg_degree:.2f}'
    
    plt.title(title, fontsize=title_fontsize, fontweight='bold', pad=20)
    plt.legend(fontsize=legend_fontsize, loc='best')
    
    if plot_config.get('show_grid', True):
        grid_alpha = plot_config.get('grid_alpha', 0.3)
        plt.grid(True, alpha=grid_alpha, linestyle='--')
    
    plt.xticks(k_values)
    
    # Add info box if requested
    if info_box_config.get('show', True) and compute_exact:
        alpha_min = min(alpha_k_values)
        alpha_max = max(alpha_k_values)
        alpha_delta = alpha_max - alpha_min
        
        info_text = f'Graph Statistics:\n'
        info_text += f'• Nodes (n): {n}\n'
        info_text += f'• Edges (m): {m}\n'
        info_text += f'• Avg Degree: {avg_degree:.2f}\n'
        info_text += f'• min(αk): {alpha_min}\n'
        info_text += f'• max(αk): {alpha_max}\n'
        info_text += f'• Δαk: {alpha_delta}'
        
        if alpha_delta == 0:
            info_text += '\n(CONSTANT)'
            box_color = info_box_config.get('constant_color', 'lightblue')
        else:
            info_text += '\n(VARYING)'
            box_color = info_box_config.get('varying_color', 'lightgreen')
        
        # Position mapping
        position = info_box_config.get('position', 'upper left')
        pos_map = {
            'upper left': (0.02, 0.98),
            'upper right': (0.98, 0.98),
            'lower left': (0.02, 0.02),
            'lower right': (0.98, 0.02)
        }
        
        x, y = pos_map.get(position, (0.02, 0.98))
        ha = 'left' if 'left' in position else 'right'
        va = 'top' if 'upper' in position else 'bottom'
        
        info_fontsize = plot_config.get('info_box_fontsize', 10)
        plt.text(x, y, info_text, transform=plt.gca().transAxes,
                fontsize=info_fontsize, verticalalignment=va,
                horizontalalignment=ha,
                bbox=dict(boxstyle='round', facecolor=box_color, alpha=0.8))
    
    # Make it look nice
    plt.tight_layout()
    
    # Determine output filename
    output_filename = config['output'].get('filename', '')
    if not output_filename:
        base_name = graph_name.replace('.txt', '').replace('.gz', '')
        output_filename = f"{base_name}_alpha_k_correlation"
    
    output_path = Path(output_dir) / f"{output_filename}.{output_format}"
    
    # Save
    plt.savefig(output_path, dpi=dpi, bbox_inches='tight')
    print(f"\n✅ Saved plot to: {output_path}")
    
    # Show if requested
    if show_plot:
        plt.show()
    
    plt.close()
    
    # Print values table if requested
    if print_table:
        print(f"\nValues:")
        print(f"{'k':<5} {'α_k(G)':<10} {'d_k(G)':<10}")
        print("-" * 30)
        for i, k in enumerate(k_values):
            if compute_exact:
                print(f"{k:<5} {alpha_k_values[i]:<10} {dk_values[i]:<10}")
            else:
                print(f"{k:<5} {'N/A':<10} {dk_values[i]:<10}")
    
    return str(output_path)


def main():
    """Main entry point."""
    # Parse command line arguments
    config_file = "config.yml"
    graph_override = None
    
    if len(sys.argv) > 1:
        if sys.argv[1] == '--graph' and len(sys.argv) > 2:
            graph_override = sys.argv[2]
        elif sys.argv[1].endswith('.yml') or sys.argv[1].endswith('.yaml'):
            config_file = sys.argv[1]
        else:
            print("Usage:")
            print("  python plot_correlation_yaml.py")
            print("  python plot_correlation_yaml.py custom_config.yml")
            print("  python plot_correlation_yaml.py --graph ca-GrQc")
            sys.exit(1)
    
    # Load configuration
    config = load_config(config_file)
    
    # Run analysis
    try:
        output_file = plot_alpha_k_correlation(config, graph_override)
        print(f"\n{'='*60}")
        print(f"✅ Analysis complete!")
        print(f"{'='*60}")
    except Exception as e:
        print(f"\n❌ Error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    # Install PyYAML if needed
    try:
        import yaml
    except ImportError:
        print("Installing PyYAML...")
        import subprocess
        subprocess.check_call([sys.executable, "-m", "pip", "install", "--user", "pyyaml"])
        import yaml
    
    main()
