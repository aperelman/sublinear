cmake_minimum_required(VERSION 3.16)
project(GraphAnalyzer LANGUAGES CXX)

set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(OpenMP REQUIRED)

include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

FetchContent_Declare(
        snap_external
        GIT_REPOSITORY https://github.com/snap-stanford/snap.git
        GIT_TAG        master
)
FetchContent_MakeAvailable(snap_external)

set(ALGO_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/algorithms/triangle_counting)
set(ARBRCTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/algorithms/exact_arboricity)
set(SNAP_ROOT   ${snap_external_SOURCE_DIR})
set(SNAP_SRC    ${SNAP_ROOT}/snap-core/Snap.cpp)
set(UI_SRC      ${CMAKE_CURRENT_SOURCE_DIR}/ui/src)

add_executable(GraphAnalyzer
        ${UI_SRC}/main.cpp
        ${UI_SRC}/mainwindow.cpp
        ${UI_SRC}/algorithm_runner.cpp
        ${UI_SRC}/download_manager.cpp
        ${UI_SRC}/snap_browser_widget.cpp
        ${UI_SRC}/snap_dataset_cache.cpp
        ${UI_SRC}/graph_list_widget.cpp
        ${UI_SRC}/graph_loader.cpp
        ${UI_SRC}/local_files_widget.cpp
        ${ALGO_DIR}/TriangleSolver.cpp
        ${ARBRCTY_DIR}/ExactArboricity.cpp
        ${SNAP_SRC}
)

# ── Skip AUTOMOC on non-Qt files so moc never sees SNAP headers ──────────────
set_source_files_properties(
        ${SNAP_SRC}
        ${ALGO_DIR}/TriangleSolver.cpp
        ${ARBRCTY_DIR}/ExactArboricity.cpp
        ${UI_SRC}/graph_loader.cpp
        PROPERTIES SKIP_AUTOMOC ON
)

# ── SNAP: compile with C++17 + permissive, skip moc ──────────────────────────
set_source_files_properties(${SNAP_SRC} PROPERTIES
        COMPILE_OPTIONS "-std=gnu++17;-fpermissive;-Wno-error"
)

# ── SNAP headers as SYSTEM so moc ignores them ───────────────────────────────
target_include_directories(GraphAnalyzer SYSTEM PRIVATE
        ${SNAP_ROOT}
        ${SNAP_ROOT}/snap-core
        ${SNAP_ROOT}/glib-core
        ${SNAP_ROOT}/snap-adv
)

# ── Non-system includes (moc will see these) ─────────────────────────────────
target_include_directories(GraphAnalyzer PRIVATE
        ${ALGO_DIR}
        ${ARBRCTY_DIR}
        ${UI_SRC}
)

set_target_properties(GraphAnalyzer PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
)

target_compile_options(GraphAnalyzer PRIVATE
        -fpermissive
        -Wno-template-id-cdtor
        -Wno-error=template-body
        -Wno-attributes
        -fopenmp
)

# ── Force C++17 + permissive on files that include Snap.h ────────────────────
set(SNAP_AFFECTED_SOURCES
        ${UI_SRC}/algorithm_runner.cpp
        ${ALGO_DIR}/TriangleSolver.cpp
        ${ARBRCTY_DIR}/ExactArboricity.cpp
)
set_source_files_properties(${SNAP_AFFECTED_SOURCES} PROPERTIES
        COMPILE_OPTIONS "-std=gnu++17;-fpermissive"
)

target_compile_definitions(GraphAnalyzer PRIVATE
        QT_NO_KEYWORDS
        GLib_OpenMP
)

target_link_libraries(GraphAnalyzer PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
        Qt6::Network
        OpenMP::OpenMP_CXX
        z
)
