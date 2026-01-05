cmake_minimum_required(VERSION 3.16)

project(WWA
    VERSION 0.1
    LANGUAGES CXX
)

# --------------------
# Qt auto tools
# --------------------
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# --------------------
# C++ standard
# --------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --------------------
# Find Qt
# --------------------
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql Network)

# --------------------
# Sources
# --------------------
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    udpreceiver.cpp
    udpreceiver.h
)

# --------------------
# Target
# --------------------
if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(WWA
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(WWA
        ${PROJECT_SOURCES}
    )
endif()

# --------------------
# Link libraries
# --------------------
target_link_libraries(WWA PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Network
)

# --------------------
# Bundle / Windows settings
# --------------------
if(QT_VERSION VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.WWA)
endif()

set_target_properties(WWA PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING
        ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

# --------------------
# Install
# --------------------
include(GNUInstallDirs)

install(TARGETS WWA
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# --------------------
# Qt 6 finalization
# --------------------
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(WWA)
endif()
