#pragma once

#include <string>
#include <vector>
#include <utility>
#include <functional>

// Load edges from a SNAP-format edge list file
std::vector<std::pair<int, int>> load_edges(
    const std::string& filePath,
    std::function<void(const std::string&)> log = nullptr);

// Load graph from file and compute exact arboricity
double calculateExactDensity(
    const std::string& filePath,
    std::function<void(const std::string&)> log = nullptr);
