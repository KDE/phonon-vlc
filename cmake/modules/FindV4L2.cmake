# - Try to find V4L2 library
# Once done this will define
#
#  V4L2_FOUND - system has V4L2
#  V4L2_INCLUDE_DIR - The V4L2 include directory
#  V4L2_LIBRARY - The library needed to use V4L2
#  V4L2_DEFINITIONS - Compiler switches required for using V4L2
#
# Copyright (C) 2008, Tanguy Krotoff <tkrotoff@gmail.com>
# Copyright (C) 2008, Lukas Durfina <lukas.durfina@gmail.com>
# Copyright (c) 2009, Fathi Boudra <fboudra@gmail.com>
# Copyright (c) 2010, Casian Andrei <skeletk13@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

if(V4L2_INCLUDE_DIR AND V4L2_LIBRARY)
   # in cache already
   set(V4L2_FIND_QUIETLY TRUE)
endif(V4L2_INCLUDE_DIR AND V4L2_LIBRARY)

set( V4L2_VERSION_OK = True )

# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
#if(NOT WIN32)
#  find_package(PkgConfig)
#  pkg_check_modules(V4L2 libvlc>=1.1.0)
#  set(V4L2_DEFINITIONS ${V4L2_CFLAGS})
#  set(V4L2_LIBRARY ${V4L2_LDFLAGS})
#  # TODO add argument support to pass version on find_package
#  include(MacroEnsureVersion)
#  macro_ensure_version(1.1.0 ${V4L2_VERSION} V4L2_VERSION_OK)
#endif(NOT WIN32)


if(V4L2_VERSION_OK)
  set(V4L2_FOUND TRUE)
  message(STATUS "V4L2 library found")
else(V4L2_VERSION_OK)
  set(V4L2_FOUND FALSE)
  message(FATAL_ERROR "V4L2 library not found")
endif(V4L2_VERSION_OK)

find_path(V4L2_INCLUDE_DIR
          NAMES libv4l2.h
          PATHS ${V4L2_INCLUDE_DIRS}
          PATH_SUFFIXES vlc)

find_library(V4L2_LIBRARY
             NAMES v4l2
             PATHS ${V4L2_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(V4L2 DEFAULT_MSG V4L2_INCLUDE_DIR V4L2_LIBRARY)

# show the V4L2_INCLUDE_DIR and V4L2_LIBRARY variables only in the advanced view
mark_as_advanced(V4L2_INCLUDE_DIR V4L2_LIBRARY)
