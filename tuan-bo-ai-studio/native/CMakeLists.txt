cmake_minimum_required(VERSION 3.21)
project(TBRetoch VERSION 0.6.0 LANGUAGES CXX)

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 QuickDialogs2 Concurrent ShaderTools)
qt_standard_project_setup(REQUIRES 6.5)
if(Qt6_VERSION VERSION_GREATER_EQUAL "6.8")
    qt_policy(SET QTP0004 NEW)
endif()

qt_add_executable(TBRetoch
    src/main.cpp
    src/AppController.cpp
    src/AppController.h
    src/LocalizationManager.cpp
    src/LocalizationManager.h
)

qt_add_qml_module(TBRetoch
    URI TBRetoch
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml/TBRetoch"
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/EditSlider.qml
)

qt_add_shaders(TBRetoch "tbretouch_shaders"
    PREFIX "/shaders"
    BASE "shaders"
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

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(TBRetochTests tests/ControllerTests.cpp src/AppController.cpp src/AppController.h)
    target_include_directories(TBRetochTests PRIVATE src)
    target_link_libraries(TBRetochTests PRIVATE Qt6::Quick Qt6::Concurrent Qt6::Test)
    add_test(NAME native-controller COMMAND TBRetochTests)
    set_tests_properties(native-controller PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 120)
endif()
