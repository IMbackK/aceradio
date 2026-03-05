cmake_minimum_required(VERSION 3.14)
project(aceradio LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt packages
find_package(Qt6 COMPONENTS Core Gui Widgets Multimedia REQUIRED)

# Note: acestep.cpp binaries and models should be provided at runtime

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

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
)

# UI file
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link libraries (only Qt libraries - acestep.cpp is external)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
)

# Include directories (only our source directory - acestep.cpp is external)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Note: acestep.cpp binaries (ace-qwen3, dit-vae) and models should be provided at runtime
