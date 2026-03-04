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
    main.cpp
    MainWindow.ui
    MainWindow.cpp
    SongListModel.cpp
    AudioPlayer.cpp
    AceStepWorker.cpp
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
