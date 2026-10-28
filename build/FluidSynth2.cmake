# MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
include_guard(GLOBAL)
set_property(GLOBAL PROPERTY MSCORE_FLUID2_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")

option(MSCORE_FLUIDSYNTH2_OPTIMIZE_DEBUG
    "Optimize modern audio DSP in Debug builds while retaining debug symbols" ON)

function(mscore_fluid2_optimize_debug target)
    if(MSCORE_FLUIDSYNTH2_OPTIMIZE_DEBUG)
        foreach(language C CXX)
            target_compile_options(${target} PRIVATE
                "$<$<AND:$<CONFIG:Debug>,$<COMPILE_LANG_AND_ID:${language},GNU,Clang,AppleClang>>:-O2>")
        endforeach()
    endif()
endfunction()

function(mscore_fluid2_optimize_debug_source source)
    if(MSCORE_FLUIDSYNTH2_OPTIMIZE_DEBUG AND CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
        set_property(SOURCE "${source}" APPEND PROPERTY COMPILE_OPTIONS
            "$<$<AND:$<CONFIG:Debug>,$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>>:-O2>")
        # The shared host PCH uses Debug's unoptimized compiler macros.
        set_property(SOURCE "${source}" PROPERTY SKIP_PRECOMPILE_HEADERS ON)
    endif()
endfunction()

function(mscore_setup_fluidsynth2)
    if(TARGET MScore::FluidSynth2)
        return()
    endif()
    get_property(_MSCORE_FLUID2_ROOT GLOBAL PROPERTY MSCORE_FLUID2_SOURCE_ROOT)
    set(MSCORE_FLUIDSYNTH2_PROVIDER "BUNDLED" CACHE STRING "FluidSynth 2 source: BUNDLED or SYSTEM")
    set_property(CACHE MSCORE_FLUIDSYNTH2_PROVIDER PROPERTY STRINGS BUNDLED SYSTEM)
    if(MSCORE_FLUIDSYNTH2_PROVIDER STREQUAL "BUNDLED")
        add_subdirectory("${_MSCORE_FLUID2_ROOT}/thirdparty/fluidsynth2"
            "${CMAKE_BINARY_DIR}/mscore-fluidsynth2" EXCLUDE_FROM_ALL)
        add_library(mscore_fluidsynth2 INTERFACE)
        target_link_libraries(mscore_fluidsynth2 INTERFACE libfluidsynth)
        set_property(TARGET mscore_fluidsynth2 PROPERTY MSCORE_FLUIDSYNTH_PROVIDER BUNDLED)
        # Upstream's subdirectory is excluded from installation; ship the
        # notice with the host rather than installing its CLI/library/headers.
        if(DEFINED Mscore_SHARE_NAME AND DEFINED Mscore_INSTALL_NAME)
            install(FILES "${_MSCORE_FLUID2_ROOT}/thirdparty/fluidsynth2/fluidsynth-2.3.4/LICENSE"
                DESTINATION "${Mscore_SHARE_NAME}${Mscore_INSTALL_NAME}/licenses"
                RENAME FluidSynth-LGPL-2.1.txt)
        endif()
    elseif(MSCORE_FLUIDSYNTH2_PROVIDER STREQUAL "SYSTEM")
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(FLUIDSYNTH2 REQUIRED IMPORTED_TARGET GLOBAL "fluidsynth>=2.3.3")
        add_library(mscore_fluidsynth2 INTERFACE)
        target_link_libraries(mscore_fluidsynth2 INTERFACE PkgConfig::FLUIDSYNTH2)
        set_property(TARGET mscore_fluidsynth2 PROPERTY MSCORE_FLUIDSYNTH_PROVIDER SYSTEM)
    else()
        message(FATAL_ERROR "MSCORE_FLUIDSYNTH2_PROVIDER must be BUNDLED or SYSTEM")
    endif()
    add_library(MScore::FluidSynth2 ALIAS mscore_fluidsynth2)
    message(STATUS "MuseScore FluidSynth 2 provider: ${MSCORE_FLUIDSYNTH2_PROVIDER}")
endfunction()
