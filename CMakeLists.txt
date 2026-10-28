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

# Module boundaries: each module owns its CMakeLists (sources, warnings, tests).
# ofgui_core is the pure C++20 model layer; ofgui_io builds on it with Qt6::Core.
add_subdirectory(src/core)
add_subdirectory(src/io)

add_library(ofgui_render STATIC
    src/render/DomainBoundsAdapter.cpp
    src/render/DomainBoundsAdapter.h
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
        ofgui_core
        Qt6::Widgets
        ${VTK_LIBRARIES}
)

add_library(ofgui_ui STATIC
    src/ui/ApplicationStyle.cpp
    src/ui/ApplicationStyle.h
    src/ui/CaseDocumentController.cpp
    src/ui/CaseDocumentController.h
    src/ui/CaseDocumentControllerDictionaries.cpp
    src/ui/CaseDocumentControllerImport.cpp
    src/ui/CaseDocumentControllerPaths.h
    src/ui/CaseDocumentControllerPersistence.cpp
    src/ui/ConfigurationPanel.cpp
    src/ui/ConfigurationPanel.h
    src/ui/DomainEditor.cpp
    src/ui/DomainEditor.h
    src/ui/DefaultCaseTemplate.cpp
    src/ui/DefaultCaseTemplate.h
    src/ui/ExportController.cpp
    src/ui/ExportController.h
    src/ui/ExportDictionaryCheck.cpp
    src/ui/ExportDictionaryCheck.h
    src/ui/ExportPreviewDialog.cpp
    src/ui/ExportPreviewDialog.h
    src/ui/ExportPreviewDialogPreview.cpp
    src/ui/ExportPreviewPanel.cpp
    src/ui/ExportPreviewPanel.h
    src/ui/FieldBoundaryTable.cpp
    src/ui/FieldBoundaryTable.h
    src/ui/FieldBoundaryTableRows.cpp
    src/ui/FieldBoundaryTableTypeRules.cpp
    src/ui/FieldEditor.cpp
    src/ui/FieldEditor.h
    src/ui/FieldEditorFields.cpp
    src/ui/FieldEditorGroups.cpp
    src/ui/InspectorPanel.cpp
    src/ui/InspectorPanel.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/MainWindowActions.cpp
    src/ui/MainWindowDocumentActions.cpp
    src/ui/MainWindowExportActions.cpp
    src/ui/NodeDetailBoundaryDetail.cpp
    src/ui/NodeDetailFormatter.cpp
    src/ui/NodeDetailFormatter.h
    src/ui/NodeDetailHtml.cpp
    src/ui/NodeDetailHtml.h
    src/ui/NodeDetailSections.h
    src/ui/ProjectNodeRef.h
    src/ui/ProjectTreeLabels.cpp
    src/ui/ProjectTreeLabels.h
    src/ui/ProjectTreeModel.cpp
    src/ui/ProjectTreeModel.h
    src/ui/ProjectTreeView.cpp
    src/ui/ProjectTreeView.h
    src/ui/SolverEditor.cpp
    src/ui/SolverEditor.h
    src/ui/SolverEditorAccessors.cpp
    src/ui/SolverEditorPaths.h
    src/ui/SolverEditorRefresh.cpp
    src/ui/SolverEditorWrites.cpp
    src/ui/UniformValueEditor.cpp
    src/ui/UniformValueEditor.h
    src/ui/UniformValueText.h
    src/ui/ValidationNavigator.cpp
    src/ui/ValidationNavigator.h
    src/ui/ValidationPanel.cpp
    src/ui/ValidationPanel.h
)

target_link_libraries(ofgui_ui
    PUBLIC
        ofgui_render
        ofgui_io
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

    add_executable(ofgui_render_scene_tests
        tests/render/DomainSceneBuilderTest.cpp
    )

    target_link_libraries(ofgui_render_scene_tests
        PRIVATE
            ofgui_render
            Qt6::Test
    )

    vtk_module_autoinit(
        TARGETS ofgui_render_scene_tests
        MODULES ${VTK_LIBRARIES}
    )

    add_test(
        NAME ofgui_render_scene_tests
        COMMAND ofgui_render_scene_tests
    )

    add_executable(ofgui_render_domain_tests
        tests/render/DomainBoundsAdapterTest.cpp
    )

    target_link_libraries(ofgui_render_domain_tests
        PRIVATE
            ofgui_render
            Qt6::Test
    )

    vtk_module_autoinit(
        TARGETS ofgui_render_domain_tests
        MODULES ${VTK_LIBRARIES}
    )

    add_test(
        NAME ofgui_render_domain_tests
        COMMAND ofgui_render_domain_tests
    )

    add_executable(ofgui_ui_domain_tests
        tests/ui/DomainEditorTest.cpp
    )

    target_link_libraries(ofgui_ui_domain_tests
        PRIVATE
            ofgui_ui
            Qt6::Test
    )

    target_compile_definitions(ofgui_ui_domain_tests
        PRIVATE
            OFGUI_FOAM_REFERENCE_DIR="${PROJECT_SOURCE_DIR}/OpenFOAM-v2206"
    )

    vtk_module_autoinit(
        TARGETS ofgui_ui_domain_tests
        MODULES ${VTK_LIBRARIES}
    )

    add_test(
        NAME ofgui_ui_domain_tests
        COMMAND ofgui_ui_domain_tests
    )

    add_executable(ofgui_ui_field_tests
        tests/ui/FieldEditorTest.cpp
    )

    target_link_libraries(ofgui_ui_field_tests
        PRIVATE
            ofgui_ui
            Qt6::Test
    )

    target_compile_definitions(ofgui_ui_field_tests
        PRIVATE
            OFGUI_FOAM_REFERENCE_DIR="${PROJECT_SOURCE_DIR}/OpenFOAM-v2206"
    )

    vtk_module_autoinit(
        TARGETS ofgui_ui_field_tests
        MODULES ${VTK_LIBRARIES}
    )

    add_test(
        NAME ofgui_ui_field_tests
        COMMAND ofgui_ui_field_tests
    )

    add_executable(ofgui_ui_smoke_tests
        tests/ui/CaseDocumentTreeTest.cpp
    )
    target_link_libraries(ofgui_ui_smoke_tests
        PRIVATE
            ofgui_ui
            Qt6::Test
    )

    target_compile_definitions(ofgui_ui_smoke_tests
        PRIVATE
            OFGUI_FOAM_REFERENCE_DIR="${PROJECT_SOURCE_DIR}/OpenFOAM-v2206"
    )

    vtk_module_autoinit(
        TARGETS ofgui_ui_smoke_tests
        MODULES ${VTK_LIBRARIES}
    )

    add_test(
        NAME ofgui_ui_smoke_tests
        COMMAND ofgui_ui_smoke_tests
    )

    add_executable(ofgui_ui_solver_tests
        tests/ui/SolverEditorTest.cpp
    )

    target_link_libraries(ofgui_ui_solver_tests
        PRIVATE
            ofgui_ui
            Qt6::Test
    )

    target_compile_definitions(ofgui_ui_solver_tests
        PRIVATE
            OFGUI_FOAM_REFERENCE_DIR="${PROJECT_SOURCE_DIR}/OpenFOAM-v2206"
    )

    vtk_module_autoinit(
        TARGETS ofgui_ui_solver_tests
        MODULES ${VTK_LIBRARIES}
    )

    add_test(
        NAME ofgui_ui_solver_tests
        COMMAND ofgui_ui_solver_tests
    )

    add_executable(ofgui_ui_workflow_tests
        tests/ui/ExportWorkflowTest.cpp
    )

    target_link_libraries(ofgui_ui_workflow_tests
        PRIVATE
            ofgui_ui
            Qt6::Test
    )

    target_compile_definitions(ofgui_ui_workflow_tests
        PRIVATE
            OFGUI_FOAM_REFERENCE_DIR="${PROJECT_SOURCE_DIR}/OpenFOAM-v2206"
    )

    vtk_module_autoinit(
        TARGETS ofgui_ui_workflow_tests
        MODULES ${VTK_LIBRARIES}
    )

    add_test(
        NAME ofgui_ui_workflow_tests
        COMMAND ofgui_ui_workflow_tests
    )
endif()

if(MSVC)
    target_compile_options(ofgui_render PRIVATE /W4 /permissive-)
    target_compile_options(ofgui_ui PRIVATE /W4 /permissive-)
    target_compile_options(ofgui_render_domain_tests PRIVATE /W4 /permissive-)
    target_compile_options(ofgui_ui_domain_tests PRIVATE /W4 /permissive-)
    target_compile_options(ofgui_ui_field_tests PRIVATE /W4 /permissive-)
    target_compile_options(ofgui_ui_smoke_tests PRIVATE /W4 /permissive-)
    target_compile_options(ofgui_ui_solver_tests PRIVATE /W4 /permissive-)
    target_compile_options(ofgui_ui_workflow_tests PRIVATE /W4 /permissive-)
    target_compile_options(OpenFOAMGUI PRIVATE /W4 /permissive-)
else()
    target_compile_options(ofgui_render PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(ofgui_ui PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(ofgui_render_domain_tests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(ofgui_ui_domain_tests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(ofgui_ui_field_tests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(ofgui_ui_smoke_tests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(ofgui_ui_solver_tests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(ofgui_ui_workflow_tests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(OpenFOAMGUI PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(OpenFOAMGUI PROPERTIES
    OUTPUT_NAME "OpenFOAMGUI"
)

qt_finalize_executable(OpenFOAMGUI)