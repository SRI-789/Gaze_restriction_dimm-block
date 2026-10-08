cmake_minimum_required(VERSION 3.21)

project(EyeAttentionQt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick WebEngineQuick WebChannel)

qt_add_executable(eye-attention-qt
    src/main.cpp
    src/AttentionController.cpp
    src/AttentionController.h
    resources.qrc
)

target_link_libraries(eye-attention-qt PRIVATE
    Qt6::Quick
    Qt6::WebEngineQuick
    Qt6::WebChannel
)

if(WIN32)
    target_link_libraries(eye-attention-qt PRIVATE Wtsapi32)
endif()
