cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(PhononVLC VERSION 0.12.0)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(PHONON_BUILD_QT5 "Build for Qt5" ON)
option(PHONON_BUILD_QT6 "Build for Qt6" ON)

# CI is stupid and doesn't allow us to set CMAKE options per build variant
if($ENV{CI_JOB_NAME_SLUG} MATCHES "qt5")
    set(PHONON_BUILD_QT5 ON)
    set(PHONON_BUILD_QT6 OFF)
elseif($ENV{CI_JOB_NAME_SLUG} MATCHES "qt6")
    set(PHONON_BUILD_QT5 OFF)
    set(PHONON_BUILD_QT6 ON)
endif()

set(VERSIONS)
if(PHONON_BUILD_QT5)
    list(APPEND VERSIONS "5")
endif()
if(PHONON_BUILD_QT6)
    list(APPEND VERSIONS "6")
    set(BUILD_WITH_QT6 ON) # necessary on older ECMs
endif()

set(QT_NO_CREATE_VERSIONLESS_TARGETS ON)
set(QT_NO_CREATE_VERSIONLESS_FUNCTIONS ON)

include(FeatureSummary)

# ECM
find_package(ECM 5.90 NO_MODULE)
set(KDE_COMPILERSETTINGS_LEVEL "5.82")
set_package_properties(ECM PROPERTIES
    TYPE REQUIRED
    DESCRIPTION "Extra CMake Modules"
    URL "https://api.kde.org/frameworks/extra-cmake-modules/html/index.html")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

# PVLC only uses static mimetype lists, they are created by cmake, hooray.
include(MimeTypes.cmake)

# LibVLC
find_package(LIBVLC 3.0.0)
set_package_properties(LIBVLC PROPERTIES
    TYPE REQUIRED
    DESCRIPTION "VLC C library"
    URL "http://git.videolan.org")

foreach(version ${VERSIONS})
    set(QT_MAJOR_VERSION ${version})

    include(KDEInstallDirs${QT_MAJOR_VERSION})
    include(ECMPoQmTools)
    include(KDECMakeSettings)
    include(KDECompilerSettings)
    include(ECMSetupVersion)

    # Phonon
    find_package(Phonon4Qt${QT_MAJOR_VERSION} 4.12.0 NO_MODULE)
    set_package_properties(Phonon4Qt${QT_MAJOR_VERSION} PROPERTIES
        TYPE REQUIRED
        DESCRIPTION "Phonon core library"
        URL "https://api.kde.org/phonon/html/index.html")

    find_package(Phonon4Qt${QT_MAJOR_VERSION}Experimental 4.12.0 NO_MODULE)
    set_package_properties(Phonon4Qt${QT_MAJOR_VERSION}Experimental PROPERTIES
        TYPE OPTIONAL
        DESCRIPTION "Phonon experimental library"
        URL "https://api.kde.org/phonon/html/index.html")
    if(Phonon4Qt${QT_MAJOR_VERSION}Experimental_FOUND)
        set(PHONON_EXPERIMENTAL TRUE)
    endif()

    ecm_setup_version(PROJECT VARIABLE_PREFIX PHONON_VLC)
    add_subdirectory(src src${version})

    unset(QUERY_EXECUTABLE CACHE)
endforeach()

ecm_install_po_files_as_qm(poqm)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
