#include "ExactArboricity.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>
#include <iostream>
#include <charconv>
#include <fstream>
#include <vector>
#include <string>
#include <functional>
#include "ExactArboricity.h"

double calculateExactDensity(const std::string& filePath, std::function<void(std::string)> log = nullptr) {
    std::vector<std::pair<int, int>> edges;
    std::ifstream file(filePath);

    if (!file.is_open()) {
        if (log) log("<span style='color: red;'>Failed to open file: " + filePath + "</span>");
        return -1.0;
    }

    if (log) log("<b>Phase 1: Loading Graph Data...</b>");

    std::string line;
    size_t lineCount = 0;

    while (std::getline(file, line)) {
        lineCount++;

        // דילוג על הערות בפורמט SNAP
        if (line.empty() || line[0] == '#') continue;

        const char* ptr = line.data();
        const char* end = line.data() + line.size();
        int u, v;

        // שימוש בשיטה האופטימלית: std::from_chars
        auto res1 = std::from_chars(ptr, end, u);
        if (res1.ec == std::errc{}) {
            ptr = res1.ptr;
            // דילוג על רווחים/טאבים בין המספרים
            while (ptr < end && (*ptr == ' ' || *ptr == '\t')) ptr++;

            auto res2 = std::from_chars(ptr, end, v);
            if (res2.ec == std::errc{}) {
                edges.emplace_back(u, v);
            }
        }

        // דיווח Verbose כל חצי מיליון שורות כדי לא להעמיס על ה-GUI
        if (lineCount % 500000 == 0 && log) {
            log("Parsed " + std::to_string(lineCount) + " lines...");
        }
    }

    if (edges.empty()) {
        if (log) log("Warning: Graph is empty.");
        return 0.0;
    }

    if (log) {
        log("Loading complete. Total edges: <b>" + std::to_string(edges.size()) + "</b>");
        log("<b>Phase 2: Computing Exact Arboricity...</b>");
    }

    // קריאה לאלגוריתם עם ה-Callback כדי לקבל דיווח על האיטרציות (Binary Search)
    ArboricityOutput result = ExactArboricity::compute(edges, log);

    if (log) log("<span style='color: green;'>Final Density Found: " + std::to_string(result.value) + "</span>");

    return result.value;
}
double calculateExactDensity(const std::string& filePath) {
    std::vector<std::pair<int, int>> edges;
    std::ifstream file(filePath);

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return -1.0;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip comments
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        int u, v;
        if (iss >> u >> v) {
            edges.emplace_back(u, v);
        }
    }

    if (edges.empty()) {
        return 0.0;
    }

    ArboricityOutput result = ExactArboricity::compute(std::move(edges), log);
    return result.max_avg_degree;
}