cmake_minimum_required(VERSION 3.16)

# Also usable alone, without Qt or a FluidSynth development installation.
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
      project(fluid2_audio_buffer_test LANGUAGES C CXX)
      enable_testing()
      option(MSCORE_EXPERIMENTAL_FLUIDSYNTH2 "Test the selected FluidSynth 2 provider" OFF)
endif()

# Native engine checks are optional in the standalone project. The full
# experimental app build already provides the selected provider target.
if (MSCORE_EXPERIMENTAL_FLUIDSYNTH2)
      include("${CMAKE_CURRENT_SOURCE_DIR}/../../../build/FluidSynth2.cmake")
      mscore_setup_fluidsynth2()
elseif (NOT TARGET MScore::FluidSynth2)
      find_package(PkgConfig QUIET)
      if (PKG_CONFIG_FOUND)
            pkg_check_modules(FLUIDSYNTH2 QUIET IMPORTED_TARGET GLOBAL "fluidsynth>=2.3.3")
            if (TARGET PkgConfig::FLUIDSYNTH2)
                  add_library(MScore::FluidSynth2 ALIAS PkgConfig::FLUIDSYNTH2)
            endif()
      endif()
endif()
set(fluid2_tests tst_fluid2_audio tst_fluid2_routing tst_fluid2_sends
      tst_auxiliary_bus tst_auxiliary_zita tst_auxiliary_settings tst_master_layout)
set(fluid2_native_tests tst_fluid2_banks tst_fluid2_return tst_fluid2_soundfonts tst_fluid2_lifecycle tst_fluid2_fontstack tst_fluid2_controllers tst_fluid2_modulators tst_fluid2_parameters)
if (TARGET MScore::FluidSynth2)
      get_target_property(fluid2_provider MScore::FluidSynth2 MSCORE_FLUIDSYNTH_PROVIDER)
      if(fluid2_provider STREQUAL "BUNDLED")
            list(APPEND fluid2_native_tests tst_fluid2_releasepitch)
            add_test(NAME tst_fluid2_vendor COMMAND "${CMAKE_COMMAND}" -P
                  "${CMAKE_CURRENT_SOURCE_DIR}/../../../thirdparty/fluidsynth2/VerifySource.cmake")
      endif()
      list(APPEND fluid2_tests ${fluid2_native_tests})
endif()

if(COMMAND mscore_fluid2_optimize_debug_source)
      mscore_fluid2_optimize_debug_source("${CMAKE_CURRENT_SOURCE_DIR}/../../../effects/zita1/zita.cpp")
endif()

foreach(fluid2_test ${fluid2_tests})
add_executable(${fluid2_test} ${fluid2_test}.cpp)
target_include_directories(${fluid2_test} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set_target_properties(${fluid2_test} PROPERTIES
      CXX_STANDARD 11
      CXX_STANDARD_REQUIRED ON
      AUTOMOC OFF
      AUTOUIC OFF
      AUTORCC OFF
      DISABLE_PRECOMPILE_HEADERS ON
      )
if (fluid2_test STREQUAL "tst_master_layout")
      # One TU explicitly enables the backend define and one disables it.
      # Use real Qt in app builds; standalone runs need only a layout bridge.
      target_sources(${fluid2_test} PRIVATE master_layout_enabled.cpp master_layout_disabled.cpp)
      if (TARGET Qt5::Core)
            target_compile_definitions(${fluid2_test} PRIVATE MS_MASTER_LAYOUT_REAL_QT)
            target_link_libraries(${fluid2_test} PRIVATE Qt5::Core)
      else()
            target_include_directories(${fluid2_test} BEFORE PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/master-layout-standalone")
      endif()
endif()
if (fluid2_test STREQUAL "tst_auxiliary_zita" OR fluid2_test STREQUAL "tst_fluid2_return" OR fluid2_test STREQUAL "tst_auxiliary_settings")
      # Compile the production DSP with a narrow Qt bridge. These targets test
      # no widgets/moc/state serialization and need no Qt SDK.
      target_include_directories(${fluid2_test} BEFORE PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/zita-standalone")
      target_sources(${fluid2_test} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../effects/zita1/zita.cpp")
endif()
if (fluid2_test STREQUAL "tst_auxiliary_settings")
      find_package(Threads REQUIRED)
      target_link_libraries(${fluid2_test} PRIVATE Threads::Threads)
endif()
if (fluid2_test IN_LIST fluid2_native_tests)
      target_sources(${fluid2_test} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../audio/midi/fluid2/synthbanks.cpp")
      target_link_libraries(${fluid2_test} PRIVATE MScore::FluidSynth2)
      if (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT fluid2_test STREQUAL "tst_fluid2_return"
            AND NOT fluid2_test STREQUAL "tst_fluid2_controllers" AND NOT fluid2_test STREQUAL "tst_fluid2_modulators"
            AND NOT fluid2_test STREQUAL "tst_fluid2_parameters" AND NOT fluid2_test STREQUAL "tst_fluid2_releasepitch")
            # Exercise second-bank load failure against the real library.
            target_compile_definitions(${fluid2_test} PRIVATE FLUID2_TEST_WRAP_LOAD)
            set_property(TARGET ${fluid2_test} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=fluid_synth_sfload")
            if (fluid2_test STREQUAL "tst_fluid2_lifecycle")
                  set_property(TARGET ${fluid2_test} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--wrap=new_fluid_synth")
            endif()
      endif()
      if (fluid2_test MATCHES "^(tst_fluid2_soundfonts|tst_fluid2_lifecycle|tst_fluid2_fontstack)$")
            add_test(NAME ${fluid2_test} COMMAND ${fluid2_test} "${CMAKE_CURRENT_BINARY_DIR}/${fluid2_test}.sf2"
                  "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/sine.ogg")
      else()
            add_test(NAME ${fluid2_test} COMMAND ${fluid2_test} "${CMAKE_CURRENT_BINARY_DIR}/${fluid2_test}.sf2")
      endif()
else()
      add_test(NAME ${fluid2_test} COMMAND ${fluid2_test})
endif()
endforeach()
