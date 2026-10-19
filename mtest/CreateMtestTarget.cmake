#=============================================================================
#  MuseScore
#  Music Composition & Notation
#
#  Copyright (C) 2011 Werner Schweer
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2
#  as published by the Free Software Foundation and appearing in
#  the file LICENSE.GPL
#=============================================================================

add_executable(
      ${TARGET}
      ${ui_headers}
      ${mocs}
      ${TARGET}.cpp
      )

if (MTEST_LINK_MSCOREAPP)
      target_link_libraries(${TARGET} testutils_mscoreapp mscoreapp)
      # GUI-backed tests run below mtest/, outside the application's bin/
      # resource layout. Stage the standard workspaces where getSharePath()
      # expects them, so startup does not depend on a system installation.
      if (UNIX AND NOT APPLE)
            get_filename_component(test_parent "${CMAKE_CURRENT_BINARY_DIR}" DIRECTORY)
            string(MD5 workspace_stage_id "${test_parent}")
            set(workspace_stage_target "mtest_workspaces_${workspace_stage_id}")
            if (NOT TARGET ${workspace_stage_target})
                  add_custom_target(${workspace_stage_target}
                        COMMAND ${CMAKE_COMMAND}
                              "-DSOURCE_DIR=${PROJECT_SOURCE_DIR}"
                              "-DBUILD_DIR=${PROJECT_BINARY_DIR}"
                              "-DRESOURCE_DIR=${test_parent}/share/${Mscore_INSTALL_NAME}"
                              -P "${PROJECT_SOURCE_DIR}/mtest/StageTestResources.cmake"
                        VERBATIM)
                  add_dependencies(${workspace_stage_target} workspaces)
                  endif()
            add_dependencies(${TARGET} ${workspace_stage_target})
            endif()
else (MTEST_LINK_MSCOREAPP)
      target_link_libraries(${TARGET} testutils)
endif (MTEST_LINK_MSCOREAPP)

if (MSVC)
      include(FindStaticLibrary)
endif (MSVC)

target_link_libraries(
      ${TARGET}
      ${QT_LIBRARIES}
      ${QT_QTTEST_LIBRARY}
      testResources
      libmscore
      mscoreapp
      audio
      qzip
      )

if (OMR)
      target_link_libraries(${TARGET} omr poppler-qt5)
      if (OCR)
            target_link_libraries(${TARGET} tesseract_api)
      endif (OCR)
endif (OMR)

# ZIP library
if (MSVC)
      target_link_libraries(
            ${TARGET}
            zlibstat
            )
else (MSVC)
      target_link_libraries(
            ${TARGET}
            z
            )
endif (MSVC)

target_link_libraries(${TARGET} freetype)

if (NOT MINGW AND NOT APPLE AND NOT MSVC)
   target_link_libraries(${TARGET}
      dl
      pthread)
endif (NOT MINGW AND NOT APPLE AND NOT MSVC)

if (APPLE)
target_link_libraries(${TARGET} ${OsxFrameworks})
target_link_libraries(${TARGET}
      dl
      pthread
      )
set_target_properties (
      ${TARGET}
      PROPERTIES
      AUTOMOC true
      COMPILE_FLAGS "-D QT_GUI_LIB -D TESTROOT=\\\"${PROJECT_SOURCE_DIR}\\\" -g -Wall -Wextra"
      LINK_FLAGS    "-g -stdlib=libc++"
      )
else(APPLE)
      if (MSVC)
            set_target_properties (
                  ${TARGET}
                  PROPERTIES
                  AUTOMOC true
                  COMPILE_FLAGS "/Zi /D QT_GUI_LIB  /D TESTROOT=\\\"${PROJECT_SOURCE_DIR}\\\""
                  )
      else (MSVC)
            set_target_properties (
                  ${TARGET}
                  PROPERTIES
                  AUTOMOC true
                  COMPILE_FLAGS "-D QT_GUI_LIB -D TESTROOT=\\\"${PROJECT_SOURCE_DIR}\\\" -g -Wall -Wextra"
                  LINK_FLAGS    "-g"
                  )
      endif (MSVC)
endif(APPLE)

if (BUILD_PCH)
      target_use_pch(${TARGET})
endif (BUILD_PCH)

add_test(${TARGET} ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}  -xunitxml -o result.xml)

# On Windows some tests need access to supporting files
# MSVC has a different definition for CMAKE_CURRENT_BINARY_DIR
# so we use this install destination variable instead
if (MSVC)
      set_target_properties(
            ${TARGET}
            PROPERTIES
            MTEST_INSTALL_DESTINATION_DIR "${CMAKE_CURRENT_BINARY_DIR}"
            )
else (MSVC)
      set_target_properties(
            ${TARGET}
            PROPERTIES
            MTEST_INSTALL_DESTINATION_DIR "${CMAKE_CURRENT_BINARY_DIR}/.."
            )
endif (MSVC)
