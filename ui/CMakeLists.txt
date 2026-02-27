cmake_minimum_required(VERSION 3.16)
project(GraphAnalyzer VERSION 1.0.0 LANGUAGES CXX)

# 1. Paths
set(VCPKG_INSTALL_PATH "C:/vcpkg/installed/x64-windows")
list(APPEND CMAKE_PREFIX_PATH ${VCPKG_INSTALL_PATH})
set(CMAKE_PREFIX_PATH "C:/Qt/6.8.3/msvc2022_64" ${CMAKE_PREFIX_PATH})

# 2. C++ settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CRITICAL FIX: Ensure proper MSVC runtime library linkage
if(MSVC)
    # For Ninja generator, ensure we're using x64 toolchain
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        message(FATAL_ERROR "32-bit build detected! Please use x64 Native Tools Command Prompt")
    endif()
    
    # Set runtime library to dynamic debug/release
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    
    # Add standard library explicitly
    add_compile_options(/EHsc /utf-8 /W3 /std:c++latest /Zc:__cplusplus /permissive-)
endif()

# 3. Qt automation
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC ON)

# 4. Find packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)
find_package(ZLIB REQUIRED)

# 5. Algorithm directories
get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
set(ALGO_BASE_DIR "${PARENT_DIR}/algorithms")
set(ARBORICITY_DIR "${ALGO_BASE_DIR}/exact_arboricity")
set(TRIANGLE_DIR   "${ALGO_BASE_DIR}/triangle_counting")

# 6. Sources and Headers (Preserving all files on your disk)
set(SOURCES
        src/main.cpp
        src/mainwindow.cpp
        src/graph_list_widget.cpp
        src/graph_loader.cpp
        src/download_manager.cpp
        src/snap_dataset_cache.cpp
        src/snap_browser_widget.cpp
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
        src/workers/triangle_counter_worker.h
        "${ARBORICITY_DIR}/density_algorithm.h"
        "${ARBORICITY_DIR}/ExactArboricity.h"
        "${TRIANGLE_DIR}/TriangleCounting.h"
)

# 7. Executable
add_executable(GraphAnalyzer ${SOURCES} ${HEADERS})

# 8. Include directories
target_include_directories(GraphAnalyzer PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/workers"
        "${PARENT_DIR}"
        "${ARBORICITY_DIR}"
        "${TRIANGLE_DIR}"
)

# 9. Link libraries - Add MSVC x64 runtime explicitly
if(MSVC)
    # Force x64 MSVC runtime libraries
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    
    # Link against x64 MSVC runtime
    target_link_options(GraphAnalyzer PRIVATE
        "/DEFAULTLIB:msvcrtd.lib"
        "/DEFAULTLIB:vcruntimed.lib"  
        "/DEFAULTLIB:ucrtd.lib"
    )
endif()

target_link_libraries(GraphAnalyzer PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        ZLIB::ZLIB
)

install(TARGETS GraphAnalyzer RUNTIME DESTINATION bin)
