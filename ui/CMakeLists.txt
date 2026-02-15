cmake_minimum_required(VERSION 3.16)
project(GraphAnalyzer VERSION 1.0.0)

# 1. הגדרת נתיבים - חשוב להגדיר לפני find_package
# מצביע לגרסה הספציפית שהתקנת עם ה-Online Installer
set(CMAKE_PREFIX_PATH "C:/Qt/6.8.3/msvc2022_64")

# 2. הגדרות C++
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 3. הגדרות אוטומטיות של Qt (MOC, UIC, RCC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 4. מציאת חבילות Qt6
find_package(Qt6 REQUIRED COMPONENTS 
    Widgets 
    Network
)

# 5. מציאת Python (בדרך כלל מגיע מ-vcpkg או מהמערכת)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# 6. רשימת קבצי מקור (Sources)
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/graph_list_widget.cpp
    src/algorithm_runner.cpp
    src/download_manager.cpp
    src/snap_dataset_cache.cpp
    src/snap_browser_widget.cpp
)

# 7. רשימת קבצי כותרת (Headers)
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

# 8. יצירת קובץ ההרצה
add_executable(GraphAnalyzer
    ${SOURCES}
    ${HEADERS}
)

# 9. קישור ספריות Qt
# השתמשנו ב-PRIVATE כי אלו תלויות פנימיות של האפליקציה
target_link_libraries(GraphAnalyzer PRIVATE
    Qt6::Widgets
    Qt6::Network
)

# 10. הגדרת נתיבי Include
target_include_directories(GraphAnalyzer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 11. הגדרות התקנה (אופציונלי)
install(TARGETS GraphAnalyzer
    RUNTIME DESTINATION bin
)