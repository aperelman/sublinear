#ifndef GRAPH_LOADER_H
#define GRAPH_LOADER_H

#include <string>
#include <vector>
#include <utility>
#include <functional>

// Declaration only - ends with a semicolon
std::vector<std::pair<int, int>> load_edges(
    const std::string& filePath,
    std::function<void(const std::string&)> progressCallback = nullptr
);

#endif