cmake_minimum_required(VERSION 3.16)

# 1. vcpkg integration — must be set BEFORE project()
if(WIN32)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static-mt" CACHE STRING "")
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    set(CMAKE_IGNORE_PATH "C:/Qt/6.8.3/msvc2022_64")
    set(CMAKE_PREFIX_PATH "C:/vcpkg/installed/x64-windows-static-mt")
    # Static CRT: MT for release, MTd for debug
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project(GraphAnalyzer VERSION 1.0.0)

# 2. GLOBAL COMPILER FIXES
if(MSVC)
    add_compile_options(/Zm200 /bigobj)
    add_definitions(-DNOMINMAX)
    # Release-mode extra optimizations
    add_compile_options($<$<CONFIG:Release>:/GL>)     # whole program optimization
    add_compile_options($<$<CONFIG:Release>:/arch:AVX2>) # AVX2 SIMD
    add_compile_options($<$<CONFIG:Release>:/fp:fast>)   # fast floating point
    add_link_options($<$<CONFIG:Release>:/LTCG>)      # link-time code generation
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options($<$<CONFIG:Release>:-O3> $<$<CONFIG:Release>:-march=native>)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 3. Find Qt6 & ZLIB
if(WIN32)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Network Gui Core EntryPointPrivate)
else()
    find_package(Qt6 REQUIRED COMPONENTS Widgets Network Gui Core)
endif()
find_package(ZLIB REQUIRED)

# 4. Setup SNAP (Source-based)
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
        snap_src
        GIT_REPOSITORY https://github.com/aperelman/snap
        GIT_TAG        compiled-by-clang21
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
        ui/src/algorithm_runner.cpp
        ui/src/download_manager.cpp
        ui/src/snap_dataset_cache.cpp
        ui/src/snap_browser_widget.cpp
        ui/src/resources.qrc
        algorithms/triangle_counting/TriangleCounting.cpp
        algorithms/common/GraphCache.cpp
        algorithms/exact_arboricity/ArboricitySolver.cpp
        algorithms/approx_arboricity/ArboricityApprox.cpp
)

add_executable(GraphAnalyzer ${SOURCES})

# All targets use C++23
set_target_properties(snap GraphAnalyzer PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON)

target_include_directories(GraphAnalyzer PRIVATE
        ui/src
        algorithms/common
        algorithms/exact_arboricity
        algorithms/approx_arboricity
        algorithms/triangle_counting
        ${snap_src_SOURCE_DIR}
        ${SNAP_CORE_DIR}
        ${snap_src_SOURCE_DIR}/glib-core
)

# 6. Link Libraries & Static Plugins
target_link_libraries(GraphAnalyzer PRIVATE
        Qt6::Widgets
        Qt6::Network
        Qt6::Gui
        Qt6::Core
        ZLIB::ZLIB
        snap
)

if(WIN32)
    target_link_libraries(GraphAnalyzer PRIVATE
            Qt6::EntryPointPrivate
            Qt6::QWindowsIntegrationPlugin
            Crypt32.lib
            Secur32.lib
            Ncrypt.lib
    )
endif()

# 7. SNAP Unicode Fix (SNAP uses narrow strings; Windows may define UNICODE globally)
if(WIN32)
    add_definitions(-UUNICODE -U_UNICODE)
endif()

