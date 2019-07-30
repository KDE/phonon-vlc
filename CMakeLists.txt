cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(PhononVLC VERSION 0.10.60)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(FeatureSummary)

# ECM
find_package(ECM 5.60 NO_MODULE)
set_package_properties(ECM PROPERTIES
    TYPE REQUIRED
    DESCRIPTION "Extra CMake Modules"
    URL "https://api.kde.org/frameworks/extra-cmake-modules/html/index.html")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(KDEInstallDirs)
include(ECMPoQmTools)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(ECMSetupVersion)

# Phonon
find_package(Phonon4Qt5 4.10.60 NO_MODULE)
set_package_properties(Phonon4Qt5 PROPERTIES
    TYPE REQUIRED
    DESCRIPTION "Phonon core library"
    URL "https://api.kde.org/phonon/html/index.html")

find_package(Phonon4Qt5Experimental 4.10.60 NO_MODULE)
set_package_properties(Phonon4Qt5Experimental PROPERTIES
    TYPE OPTIONAL
    DESCRIPTION "Phonon experimental library"
    URL "https://api.kde.org/phonon/html/index.html")
if(Phonon4Qt5Experimental_FOUND)
    set(PHONON_EXPERIMENTAL TRUE)
endif()

# LibVLC
find_package(LIBVLC 2.1.0)
set_package_properties(LIBVLC PROPERTIES
    TYPE REQUIRED
    DESCRIPTION "VLC C library"
    URL "http://git.videolan.org")

ecm_setup_version(PROJECT VARIABLE_PREFIX PHONON_VLC)

# PVLC only uses static mimetype lists, they are created by cmake, hooray.
include(MimeTypes.cmake)

add_subdirectory(src)

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/po")
    ecm_install_po_files_as_qm(po)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
