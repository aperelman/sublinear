#include "density_algorithm.h"
#include "ExactArboricity.h"
#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <charconv>

std::vector<std::pair<int, int>> load_edges(
    const std::string& filePath,
    std::function<void(const std::string&)> log)
{
    auto lg = [&](const std::string& msg) { if (log) log(msg); };

    std::vector<std::pair<int, int>> edges;
    std::ifstream file(filePath);

    if (!file.is_open()) {
        lg("<span style='color:red;'>Failed to open file: " + filePath + "</span>");
        return edges;
    }

    std::string line;
    size_t lineCount = 0;

    while (std::getline(file, line)) {
        ++lineCount;
        if (line.empty() || line[0] == '#') continue;

        const char* ptr = line.data();
        const char* end = ptr + line.size();
        int u, v;

        auto r1 = std::from_chars(ptr, end, u);
        if (r1.ec != std::errc{}) continue;
        ptr = r1.ptr;
        while (ptr < end && (*ptr == ' ' || *ptr == '\t')) ++ptr;
        auto r2 = std::from_chars(ptr, end, v);
        if (r2.ec != std::errc{}) continue;

        edges.emplace_back(u, v);

        if (lineCount % 500000 == 0)
            lg("Parsed " + std::to_string(lineCount) + " lines...");
    }

    return edges;
}

double calculateExactDensity(
    const std::string& filePath,
    std::function<void(const std::string&)> log)
{
    auto lg = [&](const std::string& msg) { if (log) log(msg); };

    lg("<b>Phase 1: Loading Graph Data...</b>");

    auto edges = load_edges(filePath, log);

    if (edges.empty()) {
        lg("Warning: No edges found in file.");
        return 0.0;
    }

    lg("Loading complete. Total edges: <b>" + std::to_string(edges.size()) + "</b>");
    lg("<b>Phase 2: Computing Exact Arboricity...</b>");

    ArboricityOutput result = ExactArboricity::compute(edges, log);

    lg("<span style='color:green;'>Arboricity: " + std::to_string(result.arboricity) + "</span>");
    return result.value;
}
