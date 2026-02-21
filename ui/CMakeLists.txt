cmake_minimum_required(VERSION 3.16)
project(GraphAnalyzer VERSION 1.0.0)

# -------------------------------------------------------------------
# 1. Set vcpkg installation path (adjust if vcpkg is elsewhere)
# -------------------------------------------------------------------
set(VCPKG_INSTALL_PATH "C:/vcpkg/installed/x64-windows")
# Add it to CMAKE_PREFIX_PATH so find_package can locate libraries
list(APPEND CMAKE_PREFIX_PATH ${VCPKG_INSTALL_PATH})

# -------------------------------------------------------------------
# 2. Qt path (adjust to your installation)
# -------------------------------------------------------------------
set(CMAKE_PREFIX_PATH "C:/Qt/6.8.3/msvc2022_64" ${CMAKE_PREFIX_PATH})

# -------------------------------------------------------------------
# 3. C++ settings
# -------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/EHsc)  # Enable C++ exceptions
    add_compile_options(/utf-8) # Set UTF-8 encoding
endif()

# Qt auto tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# -------------------------------------------------------------------
# 4. Find Qt6 components
# -------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Network
)

# -------------------------------------------------------------------
# 5. Find ZLIB (now will be found via vcpkg path)
# -------------------------------------------------------------------
find_package(ZLIB REQUIRED)

# -------------------------------------------------------------------
# 6. Algorithm directory (adjust relative path as needed)
# -------------------------------------------------------------------
set(ALGORITHM_DIR "../algorithms/exact_arboricity")

# -------------------------------------------------------------------
# 7. Source files
# -------------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/graph_list_widget.cpp
    src/download_manager.cpp
    src/snap_dataset_cache.cpp
    src/snap_browser_widget.cpp
    src/density_algorithm.cpp
    src/graph_analyzer_worker.cpp
    ${ALGORITHM_DIR}/ExactArboricity.cpp
)

# -------------------------------------------------------------------
# 8. Header files
# -------------------------------------------------------------------
set(HEADERS
    src/mainwindow.h
    src/graph_list_widget.h
    src/graph_info.h
    src/snap_catalog.h
    src/download_manager.h
    src/snap_dataset_cache.h
    src/snap_browser_widget.h
    src/density_algorithm.h
    src/graph_analyzer_worker.h
    ${ALGORITHM_DIR}/ExactArboricity.h
)

# -------------------------------------------------------------------
# 9. Create executable
# -------------------------------------------------------------------
add_executable(GraphAnalyzer ${SOURCES} ${HEADERS})

# -------------------------------------------------------------------
# 10. Link libraries
# -------------------------------------------------------------------
target_link_libraries(GraphAnalyzer PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    ZLIB::ZLIB
)

# -------------------------------------------------------------------
# 11. Include directories
# -------------------------------------------------------------------
target_include_directories(GraphAnalyzer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${ALGORITHM_DIR}
)

# -------------------------------------------------------------------
# 12. Installation (optional)
# -------------------------------------------------------------------
install(TARGETS GraphAnalyzer RUNTIME DESTINATION bin)