project(Phonon5VLC)

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

set(CMAKE_AUTOMOC ON)

find_package(ECM 0.0.11 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

################################# Requirements #################################

include(FeatureSummary)
include(GNUInstallDirs)

find_package(Phonon5 4.50.0 CONFIG REQUIRED)
message(WARNING "libvlc finder does not work terribly well with compound finding")
find_package(LIBVLC 2.0.1 REQUIRED)

list(APPEND CMAKE_MODULE_PATH ${PHONON_MODULE_DIR})
include(PhononUninstall)

################################################################################

include_directories(${PHONON_INCLUDES} ${QT_INCLUDES} ${LIBVLC_INCLUDE_DIR} ${LIBVLC_INCLUDE_DIR}/vlc/plugins)

set(PHONON_VLC_MAJOR_VERSION "4")
set(PHONON_VLC_MINOR_VERSION "50")
set(PHONON_VLC_PATCH_VERSION "0")
set(PHONON_VLC_VERSION "${PHONON_VLC_MAJOR_VERSION}.${PHONON_VLC_MINOR_VERSION}.${PHONON_VLC_PATCH_VERSION}")
add_definitions(-DPHONON_VLC_VERSION="${PHONON_VLC_VERSION}")

# PVLC only uses static mimetype lists, they are created by cmake, hooray.
include(MimeTypes.cmake)

add_subdirectory(src)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
