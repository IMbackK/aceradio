# Copyright Carl Philipp Klemm 2026
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.14)
project(aceradio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt packages
find_package(Qt6 COMPONENTS Core Gui Widgets Multimedia REQUIRED)

# Add acestep.cpp subdirectory
add_subdirectory(third_party/acestep.cpp)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Add executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/MainWindow.ui
    src/MainWindow.cpp
    src/MainWindow.h
    src/AdvancedSettingsDialog.ui
    src/AdvancedSettingsDialog.cpp
    src/AdvancedSettingsDialog.h
    src/SongDialog.ui
    src/SongDialog.cpp
    src/SongDialog.h
    src/SongListModel.cpp
    src/SongListModel.h
    src/AudioPlayer.cpp
    src/AudioPlayer.h
    src/AceStepWorker.cpp
    src/AceStepWorker.h
    src/SongItem.h
    src/clickableslider.h
    src/clickableslider.cpp
    ${MusicGeneratorGUI_H}
    res/resources.qrc
    src/elidedlabel.h src/elidedlabel.cpp
    src/SongItem.cpp

)

# UI file
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link libraries (Qt + acestep.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    acestep-core
    ggml
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/acestep.cpp/src
)

# Install targets
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# Install .desktop file
install(FILES res/xyz.uvos.aceradio.desktop DESTINATION share/applications)

# Install icon files
install(FILES res/xyz.uvos.aceradio.png DESTINATION share/icons/hicolor/256x256/apps RENAME xyz.uvos.aceradio.png)
install(FILES res/xyz.uvos.aceradio.svg DESTINATION share/icons/hicolor/scalable/apps RENAME xyz.uvos.aceradio.svg)

# Test executable
add_executable(test_acestep_worker
    tests/test_acestep_worker.cpp
    src/AceStepWorker.cpp
    src/AceStepWorker.h
    src/SongItem.cpp
    src/SongItem.h
)

target_link_libraries(test_acestep_worker PRIVATE
    Qt6::Core
    Qt6::Widgets
    acestep-core
    ggml
)

target_include_directories(test_acestep_worker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/acestep.cpp/src
)
