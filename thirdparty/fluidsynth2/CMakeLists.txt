# MuseScore - licensed under the GNU GPL version 2; see LICENSE.GPL.
cmake_minimum_required(VERSION 3.16)

# Directory-local values: do not FORCE cache entries shared with the host.
# Upstream uses C90/C++98 internally; its directory settings stay local too.
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)
set(CMAKE_UNITY_BUILD OFF)
set(MSCORE_FLUIDSYNTH_EMBEDDED ON)
# The host adds its source/build roots (and Qt) as directory-wide includes.
# Its config.h must never shadow FluidSynth's generated feature header.
# Clear inherited includes only in this dependency directory; upstream adds
# its own headers and dependency targets below.
set_property(DIRECTORY PROPERTY INCLUDE_DIRECTORIES "")
# MuseScore also has FindSndFile/FindOgg modules with different result names.
# Prefer upstream's imported-target finders within this dependency directory.
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/fluidsynth-2.3.4/cmake_admin")

# The application owns device/MIDI I/O, effects and export. Keep the SF2/SF3
# loader and synth APIs, without a second set of hardware/server backends.
foreach(feature alsa aufile coreaudio coremidi dbus dsound framework ipv6 jack
        ladspa lash libinstpatch midishare network oboe opensles oss pipewire
        portaudio pulseaudio readline sdl2 systemd wasapi waveout winmidi)
    set(enable-${feature} OFF)
endforeach()
set(enable-libsndfile ON)
set(enable-threads ON)

add_subdirectory(fluidsynth-2.3.4 upstream EXCLUDE_FROM_ALL)
get_directory_property(_fluid2_sf3 DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/fluidsynth-2.3.4"
    DEFINITION LIBSNDFILE_HASVORBIS)
if(NOT _fluid2_sf3)
    message(FATAL_ERROR "Bundled FluidSynth requires libsndfile with Ogg/Vorbis (SF3) support. Install its development package and codec dependencies; SF3 cannot be silently disabled.")
endif()
set_target_properties(libfluidsynth libfluidsynth-OBJ PROPERTIES
    POSITION_INDEPENDENT_CODE ON DISABLE_PRECOMPILE_HEADERS ON)
mscore_fluid2_optimize_debug(libfluidsynth-OBJ)
# Known diagnostics in this pinned dependency: empty device-driver tables,
# signed array-index comparisons and its deprecated compatibility wrappers.
# Keep application/adapter warnings enabled and preserve upstream source bytes.
foreach(language C CXX)
    target_compile_options(libfluidsynth-OBJ PRIVATE
        "$<$<COMPILE_LANG_AND_ID:${language},GNU,Clang,AppleClang>:-Wno-type-limits;-Wno-sign-compare;-Wno-deprecated-declarations>")
endforeach()
# Upstream attaches these through a private object-library dependency. With
# the host's CMake 3.16 policies, its link directories do not propagate across
# that boundary. Keep nonstandard dependency prefixes usable without exposing
# GLib's compiler flags to consumers of the synth headers.
foreach(package PC_GLIB2 PC_GTHREAD2 PC_SNDFILE)
    get_directory_property(_fluid2_link_dirs
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/fluidsynth-2.3.4"
        DEFINITION ${package}_LIBRARY_DIRS)
    if(_fluid2_link_dirs)
        target_link_directories(libfluidsynth INTERFACE ${_fluid2_link_dirs})
    endif()
endforeach()
