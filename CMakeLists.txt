cmake_minimum_required(VERSION 3.16)
project(GraphAnalyzer VERSION 1.0.0)

# ------------------------------------------------------------------
# Cross-platform Qt path (Windows only)
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

# If SNAP was already cloned, don't try to contact GitHub again.
# This avoids failures on networks where github.com is unreachable.
# On a fresh build (no _deps), FetchContent will still do the initial clone.
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Don't git-fetch on reconfigure")

FetchContent_Declare(
  snap_src
  GIT_REPOSITORY https://github.com/snap-stanford/snap.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(snap_src)

# After this, the source is at ${snap_src_SOURCE_DIR}
set(SNAP_CORE_DIR ${snap_src_SOURCE_DIR}/snap-core)

if(WIN32)
    # ------------------------------------------------------------------
    # Windows: compile SNAP as a single translation unit (Snap.cpp)
    #
    # SNAP is designed so that Snap.cpp #includes every other .cpp file
    # in snap-core (and glib-core).  Compiling the individual .cpp files
    # separately fails because they don't include Snap.h themselves.
    #
    # We also force C++17 for the SNAP target because SNAP's operator==
    # overloads are ambiguous under C++20's synthesised comparison rules.
    # ------------------------------------------------------------------
    add_library(snap STATIC ${SNAP_CORE_DIR}/Snap.cpp)

    # SNAP must be compiled as C++17 to avoid operator== ambiguity
    set_target_properties(snap PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )

    # Exclude snap from AUTOMOC - it has no Qt code
    set_target_properties(snap PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)

    target_include_directories(snap PUBLIC
        ${snap_src_SOURCE_DIR}
        ${SNAP_CORE_DIR}
        ${snap_src_SOURCE_DIR}/glib-core
        ${snap_src_SOURCE_DIR}/snap-adv
    )

    # Windows / MSVC compatibility defines required by SNAP headers
    target_compile_definitions(snap PUBLIC
        _USE_MATH_DEFINES   # M_PI, M_E, etc.
        NOMINMAX            # prevent Windows min/max macros
        WIN32_LEAN_AND_MEAN
    )

    # Suppress all warnings for the third-party SNAP code.
    # /bigobj is needed because the single-TU Snap.cpp produces an object
    # file that exceeds MSVC's default 65536 section limit.
    target_compile_options(snap PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W0 /wd4996 /bigobj>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-w>
    )

else()
    # ------------------------------------------------------------------
    # Linux: keep the original make / ar approach
    # ------------------------------------------------------------------
    set(SNAP_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/snap_build)
    set(SNAP_LIBRARY   ${SNAP_BUILD_DIR}/libsnap-core.a)

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
    string(JOIN " " SNAP_WARNING_FLAGS_STR ${SNAP_WARNING_FLAGS})

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

    add_library(snap STATIC IMPORTED)
    set_target_properties(snap PROPERTIES
        IMPORTED_LOCATION ${SNAP_LIBRARY}
    )
endif()

# ------------------------------------------------------------------
# Source files (from ui/src)
# ------------------------------------------------------------------
set(SOURCES
    ui/src/main.cpp
    ui/src/mainwindow.cpp
    ui/src/graph_list_widget.cpp
    ui/src/algorithm_runner.cpp
    ui/src/download_manager.cpp
    ui/src/local_files_widget.cpp
    ui/src/local_files_widget.h
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

# On Linux the SNAP library is built via a custom target; make it explicit.
if(NOT WIN32)
    add_dependencies(GraphAnalyzer build_snap)
endif()

# ------------------------------------------------------------------
# Link libraries
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
    ${snap_src_SOURCE_DIR}
    ${SNAP_CORE_DIR}
    ${snap_src_SOURCE_DIR}/glib-core
    ${snap_src_SOURCE_DIR}/snap-adv
)

# ------------------------------------------------------------------
# Optional install
# ------------------------------------------------------------------
install(TARGETS GraphAnalyzer RUNTIME DESTINATION bin)
