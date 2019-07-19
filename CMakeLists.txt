cmake_minimum_required(VERSION 2.8.9)
project(PhononVLC)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Phonon
find_package(Phonon REQUIRED)
macro_ensure_version("4.7.0" ${PHONON_VERSION} PHONON_FOUND)
macro_log_feature(PHONON_FOUND "Phonon" "Core Library" "git://anongit.kde.org/phonon.git" TRUE "4.7.0")

if(ECM_FOUND)
    include(ECMPoQmTools)
endif()

# LibVLC
find_package(LIBVLC REQUIRED)
macro_ensure_version("2.1.0" ${LIBVLC_VERSION} LIBVLC_FOUND)
macro_log_feature(LIBVLC_FOUND "LibVLC" "VLC C API Library" "git://git.videolan.org/vlc.git" TRUE "2.1.0")

if(PHONON_FOUND_EXPERIMENTAL)
  add_definitions(-DPHONON_EXPERIMENTAL)
else(PHONON_FOUND_EXPERIMENTAL)
  add_definitions(-DPHONON_VLC_NO_EXPERIMENTAL)
endif(PHONON_FOUND_EXPERIMENTAL)

add_definitions(${QT_DEFINITIONS})

include_directories(${PHONON_INCLUDES} ${QT_INCLUDES} ${LIBVLC_INCLUDE_DIR} ${LIBVLC_INCLUDE_DIR}/vlc/plugins)

set(PHONON_VLC_MAJOR_VERSION "0")
set(PHONON_VLC_MINOR_VERSION "10")
set(PHONON_VLC_PATCH_VERSION "3")
set(PHONON_VLC_VERSION "${PHONON_VLC_MAJOR_VERSION}.${PHONON_VLC_MINOR_VERSION}.${PHONON_VLC_PATCH_VERSION}")
add_definitions(-DPHONON_VLC_VERSION="${PHONON_VLC_VERSION}")

# PVLC only uses static mimetype lists, they are created by cmake, hooray.
include(MimeTypes.cmake)

add_subdirectory(src)

if(ECM_FOUND AND IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/po")
    ecm_install_po_files_as_qm(po)
endif()

macro_display_feature_log()
