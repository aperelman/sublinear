cmake_minimum_required(VERSION 3.15)
project(GraphAnalyzer VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt setup
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt5
find_package(Qt5 REQUIRED COMPONENTS Widgets Network)

# Find Python
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/graph_list_widget.cpp
    src/algorithm_runner.cpp
    src/download_manager.cpp
    src/snap_dataset_cache.cpp
    src/snap_browser_widget.cpp
)

set(HEADERS
    src/mainwindow.h
    src/graph_list_widget.h
    src/algorithm_runner.h
    src/graph_info.h
    src/snap_catalog.h
    src/download_manager.h
    src/snap_dataset_cache.h
    src/snap_browser_widget.h
)

# Create executable
add_executable(GraphAnalyzer
    ${SOURCES}
    ${HEADERS}
)

# Link Qt
target_link_libraries(GraphAnalyzer
    Qt5::Widgets
    Qt5::Network
)

# Include directories
target_include_directories(GraphAnalyzer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Install
install(TARGETS GraphAnalyzer
    RUNTIME DESTINATION bin
)
