cmake_minimum_required(VERSION 3.21)
project(TBRetoch VERSION 0.6.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 QuickDialogs2 Concurrent ShaderTools)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(TBRetoch
    src/main.cpp
    src/AppController.cpp
    src/AppController.h
    src/LocalizationManager.cpp
    src/LocalizationManager.h
)

qt_add_qml_module(TBRetoch
    URI TBRetoch
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/EditSlider.qml
)

qt_add_shaders(TBRetoch "tbretouch_shaders"
    PREFIX "/shaders"
    FILES
        shaders/color.vert
        shaders/color.frag
)

target_link_libraries(TBRetoch PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::QuickDialogs2
    Qt6::Concurrent
    Qt6::ShaderTools
)

set_target_properties(TBRetoch PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "TBRetoch"
)

install(TARGETS TBRetoch
    BUNDLE DESTINATION .
    RUNTIME DESTINATION .
)
