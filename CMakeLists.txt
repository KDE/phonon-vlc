project(Phonon)

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
set(LIBVLC_MIN_VERSION "1.1.1")
find_package(LIBVLC REQUIRED)

add_definitions(${QT_DEFINITIONS})
remove_definitions(-DQT3_SUPPORT_WARNINGS -DQT3_SUPPORT)
if(MSVC)
  add_definitions( -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS -DQT_NO_DEBUG)
  remove_definitions(-DQT_DEBUG)
endif(MSVC)

# for including config.h and for includes like <kparts/foo.h>
include_directories(${QT_INCLUDES} ${PHONON_INCLUDES})

list(APPEND PHONON_LIBS ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY})
if(QT_QTDBUS_FOUND)
    list(APPEND PHONON_LIBS ${QT_QTDBUS_LIBRARY})
endif(QT_QTDBUS_FOUND)

add_subdirectory(vlc)

macro_display_feature_log()

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
