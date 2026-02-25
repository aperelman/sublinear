#pragma once

#include <string>
#include <vector>
#include <utility>
#include <functional>

/**
 * @brief טוען רשימת קשתות מקובץ טקסט (פורמט SNAP).
 * הפונקציה הופרדה כדי לאפשר ל-Worker להשתמש באותה לוגיקה.
 */
std::vector<std::pair<int, int>> load_edges(
    const std::string& filePath,
    std::function<void(const std::string&)> log = nullptr);

/**
 * @brief טוען גרף מקובץ ומחשב אבוריסיטי מדויקת.
 */
double calculateExactDensity(
    const std::string& filePath,
    std::function<void(const std::string&)> log = nullptr);