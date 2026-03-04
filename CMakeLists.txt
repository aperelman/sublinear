cmake_minimum_required(VERSION 3.16)
project(GraphAnalyzer VERSION 1.0.0)

# ------------------------------------------------------------------
# Cross‑platform Qt path (Windows only)
# ------------------------------------------------------------------
if(WIN32 AND NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "C:/Qt/6.8.3/msvc2022_64" CACHE PATH "Qt installation prefix")
endif()

# ------------------------------------------------------------------
# C++ standard & Qt auto tools
# ------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ------------------------------------------------------------------
# Find Qt6
# ------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

# ------------------------------------------------------------------
# Download SNAP source using FetchContent
# ------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
  snap_src
  GIT_REPOSITORY https://github.com/snap-stanford/snap.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(snap_src)

# After this, the source is at ${snap_src_SOURCE_DIR}
set(SNAP_CORE_DIR ${snap_src_SOURCE_DIR}/snap-core)
set(SNAP_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/snap_build)
set(SNAP_LIBRARY   ${SNAP_BUILD_DIR}/libsnap-core.a)

# ------------------------------------------------------------------
# Suppress warnings from the SNAP library (they are harmless)
# ------------------------------------------------------------------
set(SNAP_WARNING_FLAGS
    -Wno-misleading-indentation
    -Wno-template-id-cdtor
    -Wno-unused-but-set-variable
    -Wno-catch-value
    -Wno-free-nonheap-object
    -Wno-alloc-size-larger-than
    -Wno-class-memaccess
    -Wno-maybe-uninitialized
    -Wno-parentheses
    -Wno-delete-non-virtual-dtor
    -Wno-mismatched-new-delete
    -Wno-nonnull
    -Wno-format-overflow
)

# Convert the list to a single string (space separated)
string(JOIN " " SNAP_WARNING_FLAGS_STR ${SNAP_WARNING_FLAGS})

# ------------------------------------------------------------------
# Build the SNAP core library (with warnings suppressed)
# ------------------------------------------------------------------
add_custom_target(build_snap ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SNAP_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "Building SNAP core in ${SNAP_CORE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E env "CXXFLAGS=${SNAP_WARNING_FLAGS_STR}" make -C ${SNAP_CORE_DIR}
    COMMAND ar rcs ${SNAP_CORE_DIR}/libsnap-core.a ${SNAP_CORE_DIR}/Snap.o
    COMMAND ${CMAKE_COMMAND} -E copy ${SNAP_CORE_DIR}/libsnap-core.a ${SNAP_LIBRARY}
    COMMENT "Building SNAP core library"
    BYPRODUCTS ${SNAP_LIBRARY}
    WORKING_DIRECTORY ${SNAP_CORE_DIR}
)

# Import the built library as a CMake target (only location needed)
add_library(snap STATIC IMPORTED)
set_target_properties(snap PROPERTIES
    IMPORTED_LOCATION ${SNAP_LIBRARY}
)

# ------------------------------------------------------------------
# Source files (from ui/src)
# ------------------------------------------------------------------
set(SOURCES
    ui/src/main.cpp
    ui/src/mainwindow.cpp
    ui/src/graph_list_widget.cpp
    ui/src/algorithm_runner.cpp
    ui/src/download_manager.cpp
    ui/src/snap_dataset_cache.cpp
    ui/src/snap_browser_widget.cpp
)

set(HEADERS
    ui/src/mainwindow.h
    ui/src/graph_list_widget.h
    ui/src/algorithm_runner.h
    ui/src/graph_info.h
    ui/src/snap_catalog.h
    ui/src/download_manager.h
    ui/src/snap_dataset_cache.h
    ui/src/snap_browser_widget.h
)

# ------------------------------------------------------------------
# Executable
# ------------------------------------------------------------------
add_executable(GraphAnalyzer ${SOURCES} ${HEADERS})

# Ensure the executable depends on the built SNAP library
add_dependencies(GraphAnalyzer build_snap)

# ------------------------------------------------------------------
# Link libraries – now using our imported SNAP target
# ------------------------------------------------------------------
target_link_libraries(GraphAnalyzer PRIVATE
    Qt6::Widgets
    Qt6::Network
    snap
)

# ------------------------------------------------------------------
# Include directories
# ------------------------------------------------------------------
target_include_directories(GraphAnalyzer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/src
    ${snap_src_SOURCE_DIR}       # for Snap.h
    ${SNAP_CORE_DIR}              # for Snap.h (if needed)
)

# ------------------------------------------------------------------
# Optional install
# ------------------------------------------------------------------
install(TARGETS GraphAnalyzer RUNTIME DESTINATION bin)
