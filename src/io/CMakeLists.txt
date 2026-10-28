# ---------------------------------------------------------------------------
# ofgui_io - OpenFOAM dictionary parsing/writing and project persistence.
#
# Hard boundaries (docs/ARCHITECTURE.md §2.1):
#   * depends on ofgui_core and Qt6::Core only: no Qt Widgets, no Qt GUI, no VTK.
#   * Every source file is listed explicitly; file(GLOB ...) is forbidden.
#   * Tests for this module are registered here so parallel module work never
#     has to edit the root CMakeLists.txt.
# ---------------------------------------------------------------------------

find_package(Qt6 6.5 REQUIRED COMPONENTS Core)

add_library(ofgui_io STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Version.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Version.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamSourceLocation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamToken.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamToken.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamDiagnostic.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamDiagnostic.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamParseOptions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamLexer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamLexer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamLexerChars.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamLexerTokens.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamNode.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamNode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamValue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamValue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamValueText.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamValueConversions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamParser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamParser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamParserTokens.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamParserValues.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamDictionary.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamDictionary.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamDictionaryFileReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FoamDictionaryLookup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AtomicFileWriter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AtomicFileWriter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ProjectJsonDiagnostic.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ProjectJsonDiagnostic.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ProjectJsonSchema.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ProjectJsonExtras.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ProjectJsonValueReader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ProjectJsonValueReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ProjectJsonMapping.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ProjectJsonReadSections.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ProjectJsonWrite.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ProjectJsonRead.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ProjectJsonReadPatches.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ProjectJsonReadFields.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ProjectJsonReadExtras.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ProjectJsonSource.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ProjectJsonSource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ProjectJsonRepository.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ProjectJsonRepository.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseSupportLevel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseSupportLevel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseDirectoryScan.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseDirectoryScan.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseImportOptions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseScanner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseScanner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseNameRules.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseNameRules.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseDirectoryProbe.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseDirectoryProbe.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseProjectionContext.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseFieldFile.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseFieldReader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseFieldReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseFieldApplier.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseFieldApplier.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportCodes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportIssueFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportIssueFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportedSetting.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportReport.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportReport.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BlockMeshProjector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BlockMeshProjector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BlockMeshPatchProjector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BlockMeshPatchProjector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BlockMeshValueReader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BlockMeshValueReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BoundaryConditionFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BoundaryConditionFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ControlDictProjector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ControlDictProjector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FieldProjector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FieldProjector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ReadOnlyDictionaryProjector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ReadOnlyDictionaryProjector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TransportPropertiesProjector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TransportPropertiesProjector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Sha256Hasher.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Sha256Hasher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseSourceRecorder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseSourceRecorder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseImportResult.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseImporter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CaseImporter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportDictionaryLoader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportDictionaryLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportReportBookkeeping.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportReportBookkeeping.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/FoamNumberFormat.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/FoamNumberFormat.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/FoamLayout.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/FoamLayout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/FoamDictionaryEmitter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/FoamDictionaryEmitter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/BlockMeshFaces.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/BlockMeshDictWriter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/BlockMeshDictWriter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/ControlDictWriter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/ControlDictWriter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/TransportModelSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/TransportPropertiesWriter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/TransportPropertiesWriter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/FieldFileWriter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/FieldFileWriter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/FvSchemesWriter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/FvSchemesWriter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/FvSolutionWriter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/FvSolutionWriter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/CaseExportOptions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/CaseExportPlan.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/CaseExportPlan.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/ExportAction.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/ExportAction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/ExportSkipRules.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/ExportSkipRules.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/SourceCaseWalker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/SourceCaseWalker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/CaseMirrorPlanner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/CaseMirrorPlanner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/CaseMirror.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/CaseMirror.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/ExportManifestSchema.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/ExportManifest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/ExportManifest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/ExportPlanBuilder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/ExportPlanBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/AtomicCaseDirectory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/AtomicCaseDirectory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/CaseExporter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/writers/CaseExporter.cpp
)

target_compile_features(ofgui_io PUBLIC cxx_std_20)

target_include_directories(ofgui_io
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(ofgui_io
    PUBLIC
        ofgui_core
        Qt6::Core
)

set(OFGUI_IO_WARNING_FLAGS)
if(MSVC)
    set(OFGUI_IO_WARNING_FLAGS /W4 /permissive-)
else()
    set(OFGUI_IO_WARNING_FLAGS -Wall -Wextra -Wpedantic)
endif()

target_compile_options(ofgui_io PRIVATE ${OFGUI_IO_WARNING_FLAGS})

if(BUILD_TESTING)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)

    # Test data locations are injected at configure time: tests never hard-code
    # an absolute path and never read files outside the repository.
    set(OFGUI_FOAM_FIXTURE_DIR "${PROJECT_SOURCE_DIR}/tests/fixtures/foam")
    set(OFGUI_FOAM_REFERENCE_DIR "${PROJECT_SOURCE_DIR}/OpenFOAM-v2206")

    function(ofgui_io_add_test target source)
        add_executable(${target} ${source})
        target_link_libraries(${target}
            PRIVATE
                ofgui_io
                Qt6::Test
        )
        target_compile_options(${target} PRIVATE ${OFGUI_IO_WARNING_FLAGS})
        target_compile_definitions(${target}
            PRIVATE
                OFGUI_FOAM_FIXTURE_DIR="${OFGUI_FOAM_FIXTURE_DIR}"
                OFGUI_FOAM_REFERENCE_DIR="${OFGUI_FOAM_REFERENCE_DIR}"
        )
        add_test(NAME ${target} COMMAND ${target})
    endfunction()

    ofgui_io_add_test(ofgui_io_version_tests
        ${PROJECT_SOURCE_DIR}/tests/io/VersionTest.cpp)

    ofgui_io_add_test(ofgui_io_foam_lexer_tests
        ${PROJECT_SOURCE_DIR}/tests/io/FoamLexerTest.cpp)

    ofgui_io_add_test(ofgui_io_foam_parser_tests
        ${PROJECT_SOURCE_DIR}/tests/io/FoamParserTest.cpp)

    ofgui_io_add_test(ofgui_io_foam_parser_recovery_tests
        ${PROJECT_SOURCE_DIR}/tests/io/FoamParserRecoveryTest.cpp)

    # Shared fixture/assertion helpers for the parser tests (header-only).
    target_sources(ofgui_io_foam_parser_recovery_tests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/tests/io/FoamParserTestSupport.h
    )

    ofgui_io_add_test(ofgui_io_foam_dictionary_tests
        ${PROJECT_SOURCE_DIR}/tests/io/FoamDictionaryTest.cpp)

    ofgui_io_add_test(ofgui_io_foam_reference_tests
        ${PROJECT_SOURCE_DIR}/tests/io/FoamReferenceCorpusTest.cpp)

    ofgui_io_add_test(ofgui_io_project_json_tests
        ${PROJECT_SOURCE_DIR}/tests/io/ProjectJsonRepositoryTest.cpp)

    # Sample model and field-by-field comparison helpers (header-only). Listed as
    # a source of the executable so Qt Creator / IDE builds show the header.
    target_sources(ofgui_io_project_json_tests
        PRIVATE
            ${PROJECT_SOURCE_DIR}/tests/io/ProjectJsonRepositoryTestSupport.h
    )

    ofgui_io_add_test(ofgui_io_project_json_errors_tests
        ${PROJECT_SOURCE_DIR}/tests/io/ProjectJsonRepositoryErrorsTest.cpp)

    ofgui_io_add_test(ofgui_io_project_json_source_tests
        ${PROJECT_SOURCE_DIR}/tests/io/ProjectJsonSourceTest.cpp)

    ofgui_io_add_test(ofgui_io_project_json_migration_tests
        ${PROJECT_SOURCE_DIR}/tests/io/ProjectJsonMigrationTest.cpp)

    ofgui_io_add_test(ofgui_io_case_scanner_tests
        ${PROJECT_SOURCE_DIR}/tests/io/CaseScannerTest.cpp)

    ofgui_io_add_test(ofgui_io_case_importer_tests
        ${PROJECT_SOURCE_DIR}/tests/io/CaseImporterTest.cpp)

    ofgui_io_add_test(ofgui_io_case_importer_errors_tests
        ${PROJECT_SOURCE_DIR}/tests/io/CaseImporterErrorsTest.cpp)

    ofgui_io_add_test(ofgui_io_case_importer_reference_tests
        ${PROJECT_SOURCE_DIR}/tests/io/CaseImporterReferenceTest.cpp)

    ofgui_io_add_test(ofgui_io_foam_writer_format_tests
        ${PROJECT_SOURCE_DIR}/tests/io/FoamWriterFormatTest.cpp)

    ofgui_io_add_test(ofgui_io_case_writer_golden_tests
        ${PROJECT_SOURCE_DIR}/tests/io/CaseWriterGoldenTest.cpp)

    ofgui_io_add_test(ofgui_io_case_exporter_tests
        ${PROJECT_SOURCE_DIR}/tests/io/CaseExporterTest.cpp)

    ofgui_io_add_test(ofgui_io_case_mirror_tests
        ${PROJECT_SOURCE_DIR}/tests/io/CaseMirrorTest.cpp)

    ofgui_io_add_test(ofgui_io_export_manifest_tests
        ${PROJECT_SOURCE_DIR}/tests/io/ExportManifestTest.cpp)

    ofgui_io_add_test(ofgui_io_import_code_catalog_tests
        ${PROJECT_SOURCE_DIR}/tests/io/ImportCodeCatalogTest.cpp)

    ofgui_io_add_test(ofgui_io_export_safety_tests
        ${PROJECT_SOURCE_DIR}/tests/io/ExportSafetyTest.cpp)
endif()
