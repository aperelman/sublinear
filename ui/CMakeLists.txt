cmake_minimum_required(VERSION 3.16)
project(GraphAnalyzer VERSION 1.0.0)

# 1. Paths
# Ensure these match your local environment
set(VCPKG_INSTALL_PATH "C:/vcpkg/installed/x64-windows")
list(APPEND CMAKE_PREFIX_PATH ${VCPKG_INSTALL_PATH})
set(CMAKE_PREFIX_PATH "C:/Qt/6.8.3/msvc2022_64" ${CMAKE_PREFIX_PATH})

# 2. C++ settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    # Using /std:c++latest for full C++23/26 support and UTF-8 handling
    add_compile_options(/EHsc /utf-8 /W3 /std:c++latest /Zc:__cplusplus)
endif()

# 3. Qt automation
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)  # UI is implemented in C++, no .ui files
set(CMAKE_AUTORCC ON)

# 4. Find packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)
find_package(ZLIB REQUIRED)

# 5. Algorithm directories
get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
set(ALGO_BASE_DIR "${PARENT_DIR}/algorithms")
set(ARBORICITY_DIR "${ALGO_BASE_DIR}/exact_arboricity")
set(TRIANGLE_DIR   "${ALGO_BASE_DIR}/triangle_counting")

# 6. Sources and Headers
set(SOURCES
        src/main.cpp
        src/mainwindow.cpp
        src/graph_list_widget.cpp
        src/graph_loader.cpp
        src/download_manager.cpp
        src/snap_dataset_cache.cpp
        src/snap_browser_widget.cpp
        src/workers/graph_analyzer_worker.cpp
        src/workers/triangle_counter_worker.cpp
        "${ARBORICITY_DIR}/density_algorithm.cpp"
        "${ARBORICITY_DIR}/ExactArboricity.cpp"
        "${TRIANGLE_DIR}/TriangleCounting.cpp"
)

set(HEADERS
        src/mainwindow.h
        src/graph_list_widget.h
        src/graph_loader.h
        src/graph_info.h
        src/snap_catalog.h
        src/download_manager.h
        src/snap_dataset_cache.h
        src/snap_browser_widget.h
        src/workers/graph_analyzer_worker.h
        src/workers/triangle_counter_worker.h
        "${ARBORICITY_DIR}/density_algorithm.h"
        "${ARBORICITY_DIR}/ExactArboricity.h"
        "${TRIANGLE_DIR}/TriangleCounting.h"
)

# 7. Executable
add_executable(GraphAnalyzer ${SOURCES} ${HEADERS})

# 8. Properties for Qt signals/slots
# This allows the Meta-Object Compiler to process your worker threads
set_source_files_properties(
        src/workers/graph_analyzer_worker.cpp
        src/workers/triangle_counter_worker.cpp
        src/snap_browser_widget.cpp
        PROPERTIES SKIP_AUTOMOC OFF
)

# 9. Include directories
target_include_directories(GraphAnalyzer PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${PARENT_DIR}"
        "${ARBORICITY_DIR}"
        "${TRIANGLE_DIR}"
)

# 10. Link libraries
target_link_libraries(GraphAnalyzer PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        ZLIB::ZLIB
)

# 11. Installation
install(TARGETS GraphAnalyzer RUNTIME DESTINATION bin)