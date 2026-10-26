cmake_minimum_required(VERSION 3.24)

project(OpenFOAMGUI
    VERSION 0.1.0
    DESCRIPTION "Independent Qt and VTK visual preprocessor for OpenFOAM"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include(CTest)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)
find_package(VTK 9.3 REQUIRED COMPONENTS
    CommonCore
    CommonDataModel
    FiltersCore
    FiltersSources
    InteractionStyle
    InteractionWidgets
    RenderingAnnotation
    RenderingCore
    RenderingOpenGL2
    RenderingUI
    GUISupportQt
)

add_library(ofgui_render STATIC
    src/render/DomainSceneBuilder.cpp
    src/render/DomainSceneBuilder.h
    src/render/DomainSceneActors.h
    src/render/FaceActorFactory.cpp
    src/render/FaceActorFactory.h
    src/render/FaceAppearance.cpp
    src/render/FaceAppearance.h
    src/render/FaceId.h
    src/render/FacePlaneFactory.cpp
    src/render/FacePlaneFactory.h
    src/render/OrientationMarkerController.cpp
    src/render/OrientationMarkerController.h
    src/render/OutlineActorFactory.cpp
    src/render/OutlineActorFactory.h
    src/render/PreviewDomain.cpp
    src/render/PreviewDomain.h
    src/render/ViewportCameraController.cpp
    src/render/ViewportCameraController.h
    src/render/VtkViewportWidget.cpp
    src/render/VtkViewportWidget.h
)

target_include_directories(ofgui_render
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ofgui_render
    PUBLIC
        Qt6::Widgets
        ${VTK_LIBRARIES}
)

add_library(ofgui_ui STATIC
    src/ui/ApplicationStyle.cpp
    src/ui/ApplicationStyle.h
    src/ui/ConfigurationPanel.cpp
    src/ui/ConfigurationPanel.h
    src/ui/InspectorPanel.cpp
    src/ui/InspectorPanel.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/ProjectTreeWidget.cpp
    src/ui/ProjectTreeWidget.h
)

target_link_libraries(ofgui_ui
    PUBLIC
        ofgui_render
        Qt6::Widgets
)

qt_add_executable(OpenFOAMGUI
    MANUAL_FINALIZATION
    src/app/main.cpp
    src/app/ApplicationBootstrap.cpp
    src/app/ApplicationBootstrap.h
    src/app/SmokeTestController.cpp
    src/app/SmokeTestController.h
)

target_link_libraries(OpenFOAMGUI
    PRIVATE
        ofgui_ui
)

vtk_module_autoinit(
    TARGETS OpenFOAMGUI
    MODULES ${VTK_LIBRARIES}
)

if(BUILD_TESTING)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)

    add_executable(ofgui_scene_tests
        tests/render/DomainSceneBuilderTest.cpp
    )

    target_link_libraries(ofgui_scene_tests
        PRIVATE
            ofgui_render
            Qt6::Test
    )

    vtk_module_autoinit(
        TARGETS ofgui_scene_tests
        MODULES ${VTK_LIBRARIES}
    )

    add_test(
        NAME ofgui_scene_tests
        COMMAND ofgui_scene_tests
    )
endif()

if(MSVC)
    target_compile_options(ofgui_render PRIVATE /W4 /permissive-)
    target_compile_options(ofgui_ui PRIVATE /W4 /permissive-)
    target_compile_options(OpenFOAMGUI PRIVATE /W4 /permissive-)
else()
    target_compile_options(ofgui_render PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(ofgui_ui PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(OpenFOAMGUI PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(OpenFOAMGUI PROPERTIES
    OUTPUT_NAME "OpenFOAMGUI"
)

qt_finalize_executable(OpenFOAMGUI)