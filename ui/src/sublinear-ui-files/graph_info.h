#ifndef GRAPH_INFO_H
#define GRAPH_INFO_H

#include <string>
#include <vector>
#include <filesystem>

struct GraphInfo {
    std::string name;         // Display name
    std::string filename;     // Actual file name (e.g., "wiki-Vote.txt.gz")
    std::string path;         // Full path to file (if downloaded)
    std::string url;          // Download URL
    std::string description;  // Graph description
    size_t nodes;
    size_t edges;
    bool isDownloaded;        // Whether the file exists locally
    
    // Constructor for catalog entries
    GraphInfo(const std::string& n, const std::string& f, const std::string& u, 
              const std::string& desc, size_t node_count, size_t edge_count)
        : name(n), filename(f), path(""), url(u), description(desc),
          nodes(node_count), edges(edge_count), isDownloaded(false) {}
};

inline std::string getGraphDirectory() {
    const char* home = getenv("HOME");
    return std::string(home) + "/graphs";
}

// Predefined list of popular SNAP graphs
inline std::vector<GraphInfo> getSnapGraphCatalog() {
    std::vector<GraphInfo> catalog;
    
    catalog.emplace_back("Wiki-Vote", "wiki-Vote.txt.gz",
        "https://snap.stanford.edu/data/wiki-Vote.txt.gz",
        "Wikipedia voting network", 7115, 103689);
    
    catalog.emplace_back("Email-Enron", "email-Enron.txt.gz",
        "https://snap.stanford.edu/data/email-Enron.txt.gz",
        "Email communication network", 36692, 183831);
    
    catalog.emplace_back("CA-GrQc", "ca-GrQc.txt.gz",
        "https://snap.stanford.edu/data/ca-GrQc.txt.gz",
        "Arxiv GR-QC collaboration network", 5242, 14496);
    
    catalog.emplace_back("CA-HepTh", "ca-HepTh.txt.gz",
        "https://snap.stanford.edu/data/ca-HepTh.txt.gz",
        "Arxiv HEP-TH collaboration network", 9877, 25998);
    
    catalog.emplace_back("CA-HepPh", "ca-HepPh.txt.gz",
        "https://snap.stanford.edu/data/ca-HepPh.txt.gz",
        "Arxiv HEP-PH collaboration network", 12008, 118521);
    
    catalog.emplace_back("CA-CondMat", "ca-CondMat.txt.gz",
        "https://snap.stanford.edu/data/ca-CondMat.txt.gz",
        "Arxiv condensed matter collaboration", 23133, 93497);
    
    catalog.emplace_back("CA-AstroPh", "ca-AstroPh.txt.gz",
        "https://snap.stanford.edu/data/ca-AstroPh.txt.gz",
        "Arxiv astro physics collaboration", 18772, 198110);
    
    catalog.emplace_back("P2P-Gnutella31", "p2p-Gnutella31.txt.gz",
        "https://snap.stanford.edu/data/p2p-Gnutella31.txt.gz",
        "Gnutella peer-to-peer network", 62586, 147892);
    
    catalog.emplace_back("Slashdot0811", "soc-Slashdot0811.txt.gz",
        "https://snap.stanford.edu/data/soc-Slashdot0811.txt.gz",
        "Slashdot social network", 77360, 905468);
    
    catalog.emplace_back("Epinions", "soc-Epinions1.txt.gz",
        "https://snap.stanford.edu/data/soc-Epinions1.txt.gz",
        "Epinions trust network", 75879, 508837);
    
    return catalog;
}

inline std::vector<GraphInfo> getAvailableGraphs() {
    std::vector<GraphInfo> graphs = getSnapGraphCatalog();
    std::string graphDir = getGraphDirectory();
    
    namespace fs = std::filesystem;
    
    // Check which graphs are already downloaded
    if (fs::exists(graphDir)) {
        for (auto& graph : graphs) {
            // Check for .txt file (extracted)
            std::string txtFilename = graph.filename;
            if (txtFilename.ends_with(".gz")) {
                txtFilename = txtFilename.substr(0, txtFilename.length() - 3);
            }
            
            std::string txtPath = graphDir + "/" + txtFilename;
            if (fs::exists(txtPath)) {
                graph.isDownloaded = true;
                graph.path = txtPath;
            }
        }
    }
    
    return graphs;
}

#endif // GRAPH_INFO_H
