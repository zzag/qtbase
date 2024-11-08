# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Android template files

set(build_gradle "build.gradle")
if(QT_FEATURE_developer_build)
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/${build_gradle}" build_gradle_in)
    string(REGEX REPLACE
        "\n *//noinspection GradleDependency\n"
        "\n"
        build_gradle_out
        "${build_gradle_in}"
    )
    qt_configure_file(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${build_gradle}"
        CONTENT "${build_gradle_out}"
    )
else()
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/${build_gradle}"
        "${CMAKE_CURRENT_BINARY_DIR}/${build_gradle}"
        COPYONLY
    )
endif()

set(template_files
    "${CMAKE_CURRENT_SOURCE_DIR}/AndroidManifest.xml"
    "${CMAKE_CURRENT_BINARY_DIR}/${build_gradle}")

set(template_directories
    "${CMAKE_CURRENT_SOURCE_DIR}/res")

add_custom_target(Qt${QtBase_VERSION_MAJOR}AndroidTemplates
    SOURCES
        ${template_files}
        "${CMAKE_CURRENT_SOURCE_DIR}/res/values/libs.xml"
        "${CMAKE_CURRENT_SOURCE_DIR}/res/xml/qtprovider_paths.xml"
)

qt_path_join(destination ${QT_INSTALL_DIR} ${INSTALL_DATADIR} "src/android/templates")

qt_copy_or_install(FILES ${template_files}
                   DESTINATION "${destination}")

qt_copy_or_install(DIRECTORY ${template_directories}
                   DESTINATION "${destination}")

if(NOT QT_WILL_INSTALL)
    qt_internal_copy_at_build_time(TARGET Qt${QtBase_VERSION_MAJOR}AndroidTemplates
        FILES ${template_files}
        DIRECTORIES ${template_directories}
        DESTINATION ${destination}
    )
endif()
