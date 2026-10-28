# ---------------------------------------------------------------------------
# ofgui_core - domain model layer.
#
# Hard boundaries (docs/ARCHITECTURE.md §2.1):
#   * C++20 standard library only: no Qt Widgets, no Qt GUI, no VTK, no Qt.
#   * Every source file is listed explicitly; file(GLOB ...) is forbidden.
#   * Tests for this module are registered here so parallel module work never
#     has to edit the root CMakeLists.txt.
# ---------------------------------------------------------------------------

add_library(ofgui_core STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Basics.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BoundaryCondition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BoundaryCondition.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BoundaryPatch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BoundaryPatch.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseProject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseProject.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseSourceInfo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseSourceInfo.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseValidator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseValidator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseValidatorDomain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseValidatorFields.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseValidatorMesh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseValidatorMessages.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseValidatorPatches.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseValidatorSolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DomainBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DomainBox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FaceAssignmentValidator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FaceAssignmentValidator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FaceId.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FieldDefinition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FieldDefinition.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FieldValue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FieldValue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MeshSettings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MeshSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ObjectId.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ObjectId.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ProjectSchemaVersion.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SolverSettings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SolverSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SourceLocation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SourceLocation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ValidationIssue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ValidationIssue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ValidationResult.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ValidationResult.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Vec3d.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Version.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Version.h
)

target_compile_features(ofgui_core PUBLIC cxx_std_20)

target_include_directories(ofgui_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

set(OFGUI_CORE_WARNING_FLAGS)
if(MSVC)
    set(OFGUI_CORE_WARNING_FLAGS /W4 /permissive-)
else()
    set(OFGUI_CORE_WARNING_FLAGS -Wall -Wextra -Wpedantic)
endif()

target_compile_options(ofgui_core PRIVATE ${OFGUI_CORE_WARNING_FLAGS})

# ofgui_core must stay Qt-free: no AUTOMOC/AUTOUIC/AUTORCC, so a Q_OBJECT in
# this module fails instead of silently pulling in Qt tooling.
set_target_properties(ofgui_core PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

if(BUILD_TESTING)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)

    # One executable per test file, named ofgui_<module>_<subject>_tests.
    # The test sources are pure model tests: they link ofgui_core and Qt6::Test
    # only, never VTK or Qt Widgets (docs/ARCHITECTURE.md §10.1).
    function(ofgui_add_core_test target source)
        add_executable(${target} ${PROJECT_SOURCE_DIR}/tests/core/${source})
        target_link_libraries(${target}
            PRIVATE
                ofgui_core
                Qt6::Test
        )
        target_compile_options(${target} PRIVATE ${OFGUI_CORE_WARNING_FLAGS})
        add_test(NAME ${target} COMMAND ${target})
    endfunction()

    ofgui_add_core_test(ofgui_core_version_tests VersionTest.cpp)
    ofgui_add_core_test(ofgui_core_vec3d_tests Vec3dTest.cpp)
    ofgui_add_core_test(ofgui_core_faceid_tests FaceIdTest.cpp)
    ofgui_add_core_test(ofgui_core_objectid_tests ObjectIdTest.cpp)
    ofgui_add_core_test(ofgui_core_domainbox_tests DomainBoxTest.cpp)
    ofgui_add_core_test(ofgui_core_meshsettings_tests MeshSettingsTest.cpp)
    ofgui_add_core_test(ofgui_core_boundarypatch_tests BoundaryPatchTest.cpp)
    ofgui_add_core_test(ofgui_core_fieldvalue_tests FieldValueTest.cpp)
    ofgui_add_core_test(ofgui_core_boundarycondition_tests BoundaryConditionTest.cpp)
    ofgui_add_core_test(ofgui_core_fielddefinition_tests FieldDefinitionTest.cpp)
    ofgui_add_core_test(ofgui_core_solversettings_tests SolverSettingsTest.cpp)
    ofgui_add_core_test(ofgui_core_validationissue_tests ValidationIssueTest.cpp)
    ofgui_add_core_test(ofgui_core_validationresult_tests ValidationResultTest.cpp)
    ofgui_add_core_test(ofgui_core_faceassignment_tests FaceAssignmentValidatorTest.cpp)
    ofgui_add_core_test(ofgui_core_caseproject_tests CaseProjectTest.cpp)
    ofgui_add_core_test(ofgui_core_casesourceinfo_tests CaseSourceInfoTest.cpp)
    ofgui_add_core_test(ofgui_core_casevalidator_tests CaseValidatorTest.cpp)
endif()
