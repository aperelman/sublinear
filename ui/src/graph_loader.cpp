#include "graph_loader.h"
#include <fstream>
#include <sstream>

std::vector<std::pair<int, int>> load_edges(
    const std::string& filePath,
    std::function<void(const std::string&)> progressCallback)
{
    std::vector<std::pair<int, int>> edges;
    std::ifstream file(filePath);
    std::string line;

    if (!file.is_open()) {
        if (progressCallback) progressCallback("Error: Could not open file.");
        return edges;
    }

    int count = 0;
    while (std::getline(file, line)) {
        // Skip comment lines (#) and empty lines
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        int u, v;
        if (ss >> u >> v) {
            edges.push_back({u, v});
            count++;

            // Notify UI every 100k lines to keep the app responsive
            if (count % 100000 == 0 && progressCallback) {
                progressCallback("Loaded " + std::to_string(count) + " edges...");
            }
        }
    }
    return edges;
}