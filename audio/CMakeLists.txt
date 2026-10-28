#=============================================================================
#  MuseScore
#  Music Composition & Notation
#
#  Copyright (C) 2013 Werner Schweer
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2
#  as published by the Free Software Foundation and appearing in
#  the file LICENSE.GPL
#=============================================================================

set(MODULE audio)

option(MSCORE_EXPERIMENTAL_FLUIDSYNTH2 "Build the experimental FluidSynth 2 adapter" OFF)
if(MSCORE_EXPERIMENTAL_FLUIDSYNTH2)
    include("${PROJECT_SOURCE_DIR}/build/FluidSynth2.cmake")
    mscore_setup_fluidsynth2()
endif()

include (${CMAKE_CURRENT_LIST_DIR}/midi/midi.cmake)
include (${CMAKE_CURRENT_LIST_DIR}/exports/exports.cmake)

QT5_WRAP_UI (ui_headers
    ${MIDI_UI}
    )

add_library (${MODULE} STATIC
      ${ui_headers}
      ${MIDI_SRC}
      ${EXPORTS_SRC}
      )

if(MSCORE_EXPERIMENTAL_FLUIDSYNTH2)
    # Development runs need the same bundled font which installation exposes
    # as MuseScore_General.sf3. Installed/user/extension folders retain priority.
    target_compile_definitions(${MODULE} PRIVATE
        MSCORE_BUILD_SOUNDFONT_DIR="${PROJECT_SOURCE_DIR}/share/sound")
    # Keep dependency compiler flags (such as _DEFAULT_SOURCE) off audio's
    # shared PCH consumers. Only the adapter compiles against FluidSynth.
    add_library(mscore_fluid2 STATIC midi/fluid2/fluid2.cpp midi/fluid2/fluid2.h
        midi/fluid2/fluid2gui.cpp midi/fluid2/fluid2gui.h
        midi/fluid2/audiobuffer.h midi/fluid2/channelrouting.h
        midi/fluid2/synthbanks.cpp midi/fluid2/synthbanks.h midi/fluid2/sendmix.h)
    target_include_directories(mscore_fluid2 PRIVATE ${PROJECT_SOURCE_DIR})
    target_link_libraries(mscore_fluid2 PRIVATE ${QT_LIBRARIES} MScore::FluidSynth2)
    mscore_fluid2_optimize_debug(mscore_fluid2)
    set_target_properties(mscore_fluid2 PROPERTIES
        DISABLE_PRECOMPILE_HEADERS ON
        POSITION_INDEPENDENT_CODE ON
        )
    # The optional host auxiliary return owns a separate existing Zita effect.
    target_link_libraries(${MODULE} mscore_fluid2 effects)
    target_compile_definitions(${MODULE} PUBLIC MSCORE_EXPERIMENTAL_FLUIDSYNTH2)
endif()

target_include_directories(${MODULE} 
   PRIVATE 
      ${PROJECT_SOURCE_DIR}
      ${CMAKE_CURRENT_BINARY_DIR}
      ${CMAKE_CURRENT_LIST_DIR}
      ${PROJECT_SOURCE_DIR}/global
      ${PROJECT_SOURCE_DIR}/thirdparty  # because include "mscore/musescore.h"
   )

# Do not link mscoreapp here: mscoreapp links audio, and the resulting static
# library cycle makes AUTOMOC unusable with the Xcode generator (CMake >= 4.4).
# audio's sources include mscore/*.h, but those symbols are resolved when the
# app or a test binary links mscoreapp itself.
target_link_libraries (${MODULE}
      ${QT_LIBRARIES}
      audiofile
      libmscore   # for libmscore/*.h and the freetype include dirs it carries
      preferences
      icons       # for the fluid and zerberus settings GUIs
      extension   # to find the soundfonts and sfzs extensions ship
      )

if (BUILD_PCH)
    target_use_pch(${MODULE})
endif (BUILD_PCH)
