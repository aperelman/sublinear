cmake_minimum_required(VERSION 3.16)
project(GraphAnalyzer VERSION 1.0.0)

# -------------------------------------------------------------------
# 1. Paths - adjust to your installation
# -------------------------------------------------------------------
set(VCPKG_INSTALL_PATH "C:/vcpkg/installed/x64-windows")
list(APPEND CMAKE_PREFIX_PATH ${VCPKG_INSTALL_PATH})
set(CMAKE_PREFIX_PATH "C:/Qt/6.8.3/msvc2022_64" ${CMAKE_PREFIX_PATH})

# -------------------------------------------------------------------
# 2. Stale file guard - delete files that must not exist in ui/src
# -------------------------------------------------------------------
foreach(STALE_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/density_algorithm.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/density_algorithm.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/graph_analyzer_worker.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/graph_analyzer_worker.cpp")
    if(EXISTS "${STALE_FILE}")
        message(STATUS "Removing stale file: ${STALE_FILE}")
        file(REMOVE "${STALE_FILE}")
    endif()
endforeach()

# -------------------------------------------------------------------
# 3. C++ settings
# -------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/EHsc /utf-8 /W3 /std:c++latest)
endif()

# -------------------------------------------------------------------
# 4. Qt automation
# -------------------------------------------------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# -------------------------------------------------------------------
# 5. Find packages
# -------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)
find_package(ZLIB REQUIRED)

# -------------------------------------------------------------------
# 6. Algorithm directories
# -------------------------------------------------------------------
set(ARBORICITY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../algorithms/exact_arboricity")
set(TRIANGLE_DIR   "${CMAKE_CURRENT_SOURCE_DIR}/../algorithms/triangle_counting")

# -------------------------------------------------------------------
# 7. Sources
# -------------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/graph_list_widget.cpp
    src/download_manager.cpp
    src/snap_dataset_cache.cpp
    src/snap_browser_widget.cpp
    src/workers/graph_analyzer_worker.cpp
    src/workers/triangle_counter_worker.cpp
    ${ARBORICITY_DIR}/density_algorithm.cpp
    ${ARBORICITY_DIR}/ExactArboricity.cpp
    ${TRIANGLE_DIR}/TriangleCounting.cpp
)

set(HEADERS
    src/mainwindow.h
    src/graph_list_widget.h
    src/graph_info.h
    src/snap_catalog.h
    src/download_manager.h
    src/snap_dataset_cache.h
    src/snap_browser_widget.h
    src/workers/graph_analyzer_worker.h
    src/workers/triangle_counter_worker.h
    ${ARBORICITY_DIR}/density_algorithm.h
    ${ARBORICITY_DIR}/ExactArboricity.h
    ${TRIANGLE_DIR}/TriangleCounting.h
)

# -------------------------------------------------------------------
# 8. Executable
# -------------------------------------------------------------------
add_executable(GraphAnalyzer ${SOURCES} ${HEADERS})

# Skip MOC for .cpp files - Q_OBJECT is in the headers only
set_source_files_properties(
    src/workers/graph_analyzer_worker.cpp
    src/workers/triangle_counter_worker.cpp
    src/snap_browser_widget.cpp
    PROPERTIES SKIP_AUTOMOC ON
)

# -------------------------------------------------------------------
# 9. Include directories
# -------------------------------------------------------------------
target_include_directories(GraphAnalyzer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/workers
    ${ARBORICITY_DIR}
    ${TRIANGLE_DIR}
)

# -------------------------------------------------------------------
# 10. Link libraries
# -------------------------------------------------------------------
target_link_libraries(GraphAnalyzer PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    ZLIB::ZLIB
)

install(TARGETS GraphAnalyzer RUNTIME DESTINATION bin)
