cmake_minimum_required(VERSION 3.16)

# 1. FORCE vcpkg STATIC
if(WIN32)
    set(VCPKG_TARGET_TRIPLET x64-windows-static-md CACHE STRING "")
    set(CMAKE_IGNORE_PATH "C:/Qt/6.8.3/msvc2022_64")
    set(CMAKE_PREFIX_PATH "C:/vcpkg/installed/x64-windows-static-md")
    # Matches the vcpkg default runtime (Dynamic CRT)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

project(GraphAnalyzer VERSION 1.0.0)

# 2. GLOBAL COMPILER FIXES
if(MSVC)
    add_compile_options(/Zm200 /bigobj)
    # FIX FOR C2589: Disables Windows min/max macros to allow std::numeric_limits
    add_definitions(-DNOMINMAX)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 3. Find Qt6 & ZLIB
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Gui Core EntryPointPrivate)
find_package(ZLIB REQUIRED)

# 4. Setup SNAP (Source-based)
include(FetchContent)
FetchContent_Declare(
        snap_src
        GIT_REPOSITORY https://github.com/aperelman/snap
        GIT_TAG       compiled-by-clang21
)
FetchContent_MakeAvailable(snap_src)
set(SNAP_CORE_DIR ${snap_src_SOURCE_DIR}/snap-core)

add_library(snap STATIC ${SNAP_CORE_DIR}/Snap.cpp)
target_include_directories(snap PUBLIC
        ${snap_src_SOURCE_DIR}
        ${SNAP_CORE_DIR}
        ${snap_src_SOURCE_DIR}/glib-core
)

# 5. Source & Header Files
set(SOURCES
        ui/src/main.cpp
        ui/src/mainwindow.cpp
        ui/src/graph_list_widget.cpp
        ui/src/graph_loader.cpp
        ui/src/local_files_widget.cpp
        ui/src/algorithm_runner.cpp
        ui/src/download_manager.cpp
        ui/src/snap_dataset_cache.cpp
        ui/src/snap_browser_widget.cpp
        algorithms/exact_arboricity/ExactArboricity.cpp
        algorithms/triangle_counting/TriangleCounting.cpp
        algorithms/common/GraphCache.cpp
        algorithms/exact_arboricity/ArboricitySolver.cpp
        algorithms/exact_arboricity/density_algorithm.cpp
)

add_executable(GraphAnalyzer ${SOURCES})

# All targets use C++23
set_target_properties(snap GraphAnalyzer PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON)

target_include_directories(GraphAnalyzer PRIVATE
        ui/src
        algorithms/common
        algorithms/exact_arboricity
        algorithms/triangle_counting
        ${snap_src_SOURCE_DIR}
        ${SNAP_CORE_DIR}
        ${snap_src_SOURCE_DIR}/glib-core
)

# 6. Link Libraries & Static Plugins
# NOTE: We removed the hardcoded .lib paths that were missing
target_link_libraries(GraphAnalyzer PRIVATE
        Qt6::Widgets
        Qt6::Network
        Qt6::Gui
        Qt6::Core
        Qt6::EntryPointPrivate
        Qt6::QWindowsIntegrationPlugin
        ZLIB::ZLIB
        snap
        # REQUIRED for HTTPS support on Windows without separate plugins
        Crypt32.lib
        Secur32.lib
        Ncrypt.lib
)

# 7. SNAP Unicode Fix
if(WIN32)
    add_definitions(-UUNICODE -U_UNICODE)
endif()