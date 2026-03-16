cmake_minimum_required(VERSION 3.16)
project(GraphAnalyzer VERSION 1.0.0)

# ------------------------------------------------------------------
# Cross-platform search paths (Windows only)
# ------------------------------------------------------------------
if(WIN32)
    list(APPEND CMAKE_PREFIX_PATH
        "C:/Qt/6.8.3/msvc2022_64"
        "C:/vcpkg/installed/x64-windows"
    )
    set(ZLIB_INCLUDE_DIR "C:/vcpkg/installed/x64-windows/include" CACHE PATH "")
    set(ZLIB_LIBRARY     "C:/vcpkg/installed/x64-windows/lib/zlib.lib" CACHE FILEPATH "")
endif()

# ------------------------------------------------------------------
# C++ standard & Qt auto tools
# ------------------------------------------------------------------
if(WIN32)
    set(CMAKE_CXX_STANDARD 17)
else()
    set(CMAKE_CXX_STANDARD 23)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD_LATEST OFF CACHE BOOL "" FORCE)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ------------------------------------------------------------------
# Find Qt6
# ------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

if(WIN32)
    foreach(_qt_target Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network)
        if(TARGET ${_qt_target})
            set_target_properties(${_qt_target} PROPERTIES
                INTERFACE_COMPILE_FEATURES ""
            )
        endif()
    endforeach()
endif()

# ------------------------------------------------------------------
# Find ZLIB
# ------------------------------------------------------------------
find_package(ZLIB REQUIRED)

# ------------------------------------------------------------------
# Download SNAP source using FetchContent
# ------------------------------------------------------------------
include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Don't git-fetch on reconfigure")
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
  snap_src
  GIT_REPOSITORY https://github.com/snap-stanford/snap.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(snap_src)

set(SNAP_CORE_DIR ${snap_src_SOURCE_DIR}/snap-core)

if(WIN32)
    add_library(snap STATIC ${SNAP_CORE_DIR}/Snap.cpp)

    set_target_properties(snap PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        AUTOMOC OFF AUTOUIC OFF AUTORCC OFF
    )

    target_include_directories(snap PUBLIC
        ${snap_src_SOURCE_DIR}
        ${SNAP_CORE_DIR}
        ${snap_src_SOURCE_DIR}/glib-core
        ${snap_src_SOURCE_DIR}/snap-adv
    )

    target_compile_definitions(snap PUBLIC
        _USE_MATH_DEFINES
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )

    target_compile_options(snap PRIVATE
        /W0 /wd4996 /bigobj /std:c++17
    )

else()
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
# Source files
# ------------------------------------------------------------------
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
    algorithms/exact_arboricity/ArboricitySolver.cpp
    algorithms/exact_arboricity/density_algorithm.cpp
)

set(HEADERS
    ui/src/mainwindow.h
    ui/src/graph_list_widget.h
    ui/src/graph_loader.h
    ui/src/local_files_widget.h
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

if(NOT WIN32)
    add_dependencies(GraphAnalyzer build_snap)
endif()

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

if(WIN32)
    target_include_directories(GraphAnalyzer PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/algorithms/exact_arboricity
    )
endif()

# ------------------------------------------------------------------
# Windows: force C++17, fix UNICODE conflict with SNAP headers
# ------------------------------------------------------------------
if(WIN32)
    target_compile_options(GraphAnalyzer PRIVATE /std:c++17)

    set_source_files_properties(
        ui/src/algorithm_runner.cpp
        ui/src/snap_browser_widget.cpp
        ui/src/snap_dataset_cache.cpp
        algorithms/exact_arboricity/ExactArboricity.cpp
        algorithms/exact_arboricity/ArboricitySolver.cpp
        algorithms/exact_arboricity/density_algorithm.cpp
        PROPERTIES COMPILE_OPTIONS "/UUNICODE;/U_UNICODE"
    )
endif()

# ------------------------------------------------------------------
# Link libraries
# ------------------------------------------------------------------
target_link_libraries(GraphAnalyzer PRIVATE
    Qt6::Widgets
    Qt6::Network
    ZLIB::ZLIB
    snap
)

install(TARGETS GraphAnalyzer RUNTIME DESTINATION bin)
