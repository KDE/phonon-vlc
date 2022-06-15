if (MSVC OR (WIN32 AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel"))
    set (CMAKE_MODULE_PATH ${CMAKE_INSTALL_PREFIX}/share/apps/cmake/modules)
    find_package(KDEWin)
        if (KDEWIN_FOUND)
            include_directories(${KDEWIN_INCLUDES}/msvc)
            link_libraries(${KDEWIN_LIBRARIES})
        else (KDEWIN_FOUND)
            include(CheckIncludeFileCXX)
            check_include_file_cxx(inttypes.h HAVE_INTTYPES)
            check_include_file_cxx(stdint.h   HAVE_STDINT)
            if ( NOT HAVE_STDINT AND NOT HAVE_INTTYPES )
                message (FATAL_ERROR "You don't have stdint.h and inttypes.h\n\t get them from  http://code.google.com/p/baseutils/source/browse/#svn/trunk/msvc,\n\t or get kdewin http://websvn.kde.org/trunk/kdesupport/kdewin/")
            endif ( NOT HAVE_STDINT AND NOT HAVE_INTTYPES )
        endif (KDEWIN_FOUND)
endif (MSVC OR (WIN32 AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel"))

# NB: This is evaluated in the build system because the phonon headers
# change behavior based on the defines, meaning the defines must already be set
# by the time the headers are pre-processed!
if(${PHONON_VERSION} VERSION_GREATER "4.9.50")
    message(STATUS "Building against Phonon 4.10 API")
    set(BACKEND_VERSION_DEFINE -DPHONON_BACKEND_VERSION_4_10)
# Older not supported because backend doesn't build with older cmake rigging anwyay.
endif()
add_definitions(${BACKEND_VERSION_DEFINE}) # also automatically used for moc

set(phonon_vlc_SRCS
    audio/audiooutput.cpp
    audio/audiodataoutput.cpp
    audio/volumefadereffect.cpp
    backend.cpp
    devicemanager.cpp
    effect.cpp
    effectmanager.cpp
    media.cpp
    mediacontroller.cpp
    mediaobject.cpp
    mediaplayer.cpp
    sinknode.cpp
    streamreader.cpp
#    video/videodataoutput.cpp
    video/videowidget.cpp
    video/videomemorystream.cpp
    utils/debug.cpp
    utils/libvlc.cpp

    audio/audiooutput.h
    audio/audiodataoutput.h
    audio/volumefadereffect.h
    backend.h
    devicemanager.h
    effect.h
    effectmanager.h
    media.h
    mediacontroller.h
    mediaobject.h
    mediaplayer.h
    sinknode.h
    streamreader.h
#    video/videodataoutput.cpp
    video/videowidget.h
    video/videomemorystream.h
    utils/debug.h
    utils/libvlc.h
)

if(NOT ${LIBVLC_VERSION} VERSION_LESS "2.2.0")
    list(APPEND phonon_vlc_SRCS equalizereffect.cpp equalizereffect.h)
endif()

if(PHONON_EXPERIMENTAL)
    list(APPEND phonon_vlc_SRCS video/videodataoutput.cpp video/videodataoutput.h)
endif()

if(APPLE)
    list(APPEND phonon_vlc_SRCS
        video/mac/nsvideoview.mm
        video/mac/vlcmacwidget.mm)
endif()

ecm_create_qm_loader(phonon_vlc_SRCS phonon_vlc_qt)
add_library(phonon_vlc MODULE ${phonon_vlc_SRCS})

target_include_directories(phonon_vlc
    PRIVATE
        # Extra include. We use some internal stuff for easy chroma conversion.
        ${LIBVLC_INCLUDE_DIR}/vlc/plugins
)

target_link_libraries(phonon_vlc
    Phonon::phonon4qt${QT_MAJOR_VERSION}
    Qt::Core
    Qt::Widgets
    LibVLC::Core
    LibVLC::LibVLC
)
if(PHONON_EXPERIMENTAL)
    target_link_libraries(phonon_vlc Phonon::phonon4qt${QT_MAJOR_VERSION}experimental)
endif()

install(TARGETS phonon_vlc DESTINATION ${PHONON_BACKEND_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/utils/mime.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/utils/mime.h @ONLY)

# Instead of desktop files we are embedding the information into the plugin itself.
# We have no KDE technology to help with finding the actual libraries anyway, so
# we need to have the library path anyway.
# Also see qtplugin/Q_PLUGIN_METADATA documentation.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/phonon-vlc.json.in
                ${CMAKE_CURRENT_BINARY_DIR}/phonon-vlc.json @ONLY)
