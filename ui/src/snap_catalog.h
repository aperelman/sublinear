#ifndef SNAP_CATALOG_H
#define SNAP_CATALOG_H

#include <string>
#include <vector>

struct SNAPDataset {
    std::string name;
    std::string url;
    std::string description;
};

class SNAPCatalog {
public:
    static const std::vector<SNAPDataset>& get() {
        static std::vector<SNAPDataset> catalog = {
            {"Facebook", "https://snap.stanford.edu/data/facebook_combined.txt.gz", "Social circles from Facebook"},
            {"Twitter", "https://snap.stanford.edu/data/twitter_combined.txt.gz", "Twitter networks"},
            {"Wiki-Vote", "https://snap.stanford.edu/data/wiki-Vote.txt.gz", "Wikipedia voting network"}
        };
        return catalog;
    }
};

#endif