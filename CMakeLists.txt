project(PhononVLC)

cmake_minimum_required(VERSION 2.6.2 FATAL_ERROR)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# Phonon
find_package(Phonon REQUIRED)
if(PHONON_PULSESUPPORT)
  add_definitions(-DPHONON_PULSESUPPORT)
endif(PHONON_PULSESUPPORT)
if(PHONON_FOUND_EXPERIMENTAL)
  add_definitions(-DPHONON_EXPERIMENTAL)
else(PHONON_FOUND_EXPERIMENTAL)
  add_definitions(-DPHONON_VLC_NO_EXPERIMENTAL)
endif(PHONON_FOUND_EXPERIMENTAL)

# libVLC
set(LIBVLC_MIN_VERSION "2.0.1")
find_package(LIBVLC REQUIRED)

add_definitions(${QT_DEFINITIONS})

include_directories(${PHONON_INCLUDES} ${QT_INCLUDES} ${LIBVLC_INCLUDE_DIR} ${LIBVLC_INCLUDE_DIR}/vlc/plugins)

set(PHONON_VLC_MAJOR_VERSION "0")
set(PHONON_VLC_MINOR_VERSION "6")
set(PHONON_VLC_PATCH_VERSION "3")
set(PHONON_VLC_VERSION "${PHONON_VLC_MAJOR_VERSION}.${PHONON_VLC_MINOR_VERSION}.${PHONON_VLC_PATCH_VERSION}")
add_definitions(-DPHONON_VLC_VERSION="${PHONON_VLC_VERSION}")

add_subdirectory(src)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/phonon-backend-vlc-${PHONON_VLC_VERSION}.tar.xz
    COMMAND
        git archive --prefix=phonon-backend-vlc-${PHONON_VLC_VERSION}/ HEAD | xz -9 > ${CMAKE_BINARY_DIR}/phonon-backend-vlc-${PHONON_VLC_VERSION}.tar.xz
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(
    dist
    DEPENDS ${CMAKE_BINARY_DIR}/phonon-backend-vlc-${PHONON_VLC_VERSION}.tar.xz
)
