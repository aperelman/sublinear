#pragma once

#include <string>

/**
 * @brief Wrapper function to calculate triangles from a file
 * 
 * This function loads edges from a file and runs the importance-sampling
 * triangle counting algorithm. It uses the arboricity value passed in.
 * 
 * @param filePath Path to edge list file (format: "u v" per line)
 * @param arboricity Graph arboricity value (from exact calculation)
 * @return Estimated number of triangles
 */
double calculateTriangleCount(const std::string& filePath, double arboricity);
