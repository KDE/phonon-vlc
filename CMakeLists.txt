cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(PhononVLC VERSION 0.11.3)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(FeatureSummary)

# ECM
find_package(ECM 5.90 NO_MODULE)
set(KDE_COMPILERSETTINGS_LEVEL "5.82")
set_package_properties(ECM PROPERTIES
    TYPE REQUIRED
    DESCRIPTION "Extra CMake Modules"
    URL "https://api.kde.org/frameworks/extra-cmake-modules/html/index.html")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(ECMPoQmTools)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(ECMSetupVersion)

# Phonon
find_package(Phonon4Qt${QT_MAJOR_VERSION} 4.10.60 NO_MODULE)
set_package_properties(Phonon4Qt${QT_MAJOR_VERSION} PROPERTIES
    TYPE REQUIRED
    DESCRIPTION "Phonon core library"
    URL "https://api.kde.org/phonon/html/index.html")

find_package(Phonon4Qt${QT_MAJOR_VERSION}Experimental 4.10.60 NO_MODULE)
set_package_properties(Phonon4Qt${QT_MAJOR_VERSION}Experimental PROPERTIES
    TYPE OPTIONAL
    DESCRIPTION "Phonon experimental library"
    URL "https://api.kde.org/phonon/html/index.html")
if(Phonon4Qt${QT_MAJOR_VERSION}Experimental_FOUND)
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

ecm_install_po_files_as_qm(poqm)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
