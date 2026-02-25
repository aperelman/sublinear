#include "density_algorithm.h"
#include "ExactArboricity.h"
#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <charconv>
#include <cctype>

std::vector<std::pair<int, int>> load_edges(
    const std::string& filePath,
    std::function<void(const std::string&)> log)
{
    auto lg = [&](const std::string& msg) { if (log) log(msg); };
    std::vector<std::pair<int, int>> edges;

    // שימוש ב-ifstream עם Buffer גדול יותר לשיפור ביצועי קריאה מהדיסק
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        lg("<span style='color:red;'>Failed to open file: " + filePath + "</span>");
        return edges;
    }

    // הקצאת זיכרון ראשונית למניעת Reallocations תכופים (משפר מהירות)
    edges.reserve(1000000);

    std::string line;
    size_t lineCount = 0;

    while (std::getline(file, line)) {
        ++lineCount;

        if (line.empty()) continue;

        const char* ptr = line.data();
        const char* end = ptr + line.size();

        // 1. דילוג על רווחים/הערות בתחילת שורה
        while (ptr < end && std::isspace(static_cast<unsigned char>(*ptr))) ptr++;
        if (ptr == end || *ptr == '#') continue;

        int u, v;

        // 2. קריאת המספר הראשון (מהיר מאוד, ללא הקצאות זיכרון)
        auto r1 = std::from_chars(ptr, end, u);
        if (r1.ec != std::errc{}) continue;

        // 3. דילוג על רווחים בין המספרים
        ptr = r1.ptr;
        while (ptr < end && std::isspace(static_cast<unsigned char>(*ptr))) ptr++;
        if (ptr == end) continue;

        // 4. קריאת המספר השני
        auto r2 = std::from_chars(ptr, end, v);
        if (r2.ec != std::errc{}) continue;

        edges.emplace_back(u, v);

        if (lineCount % 2000000 == 0) {
            lg("Parsed " + std::to_string(lineCount) + " lines...");
        }
    }

    // שחרור זיכרון עודף אם הוקצה יותר מדי ב-reserve
    edges.shrink_to_fit();
    return edges;
}

double calculateExactDensity(
    const std::string& filePath,
    std::function<void(const std::string&)> log)
{
    auto lg = [&](const std::string& msg) { if (log) log(msg); };
    lg("<b>Phase 1: Loading Graph Data (Optimized)...</b>");

    auto edges = load_edges(filePath, log);

    if (edges.empty()) {
        lg("<span style='color:orange;'>Warning: No edges found. Check file encoding/format.</span>");
        return 0.0;
    }

    lg("Loaded <b>" + std::to_string(edges.size()) + "</b> edges.");
    lg("<b>Phase 2: Computing Exact Arboricity...</b>");

    ArboricityOutput result = ExactArboricity::compute(edges, log);
    return result.value;
}