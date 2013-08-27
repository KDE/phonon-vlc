include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

add_definitions(-DPHONON_LIB_INSTALL_DIR="${LIB_INSTALL_DIR}")

if (MSVC)
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
endif (MSVC)

if(${PHONON_VERSION} VERSION_GREATER "4.6.49") # Pre-release 4.7 starts at 4.6.50
    message("Building against Phonon 4.7 API")
    add_definitions(-DPHONON_BACKEND_VERSION_4_7)
elseif(${PHONON_VERSION} VERSION_GREATER "4.1.99")
    message("Building against Phonon 4.2 API")
    add_definitions(-DPHONON_BACKEND_VERSION_4_2)
else()
    message("Building against Phonon 4.0 API")
endif()

set(phonon_vlc_SRCS
    audio/audiooutput.cpp
#    audio/audiodataoutput.cpp
#    audio/volumefadereffect.cpp
    backend.cpp
    devicemanager.cpp
#    effect.cpp
#    effectmanager.cpp
    media.cpp
    mediaobject.cpp
    mediaplayer.cpp
    sinknode.cpp
#    streamreader.cpp
#    video/videodataoutput.cpp
#    video/videographicsobject.cpp
#    video/videowidget.cpp
#    video/videomemorystream.cpp
    utils/debug.cpp
    utils/libvlc.cpp
)

if(APPLE)
    list(APPEND phonon_vlc_SRCS
        video/mac/nsvideoview.mm
        video/mac/vlcmacwidget.mm)
endif(APPLE)

add_library(phonon_vlc MODULE ${phonon_vlc_SRCS})
qt5_use_modules(phonon_vlc Core Widgets)

set_target_properties(phonon_vlc PROPERTIES PREFIX "")
target_link_libraries(phonon_vlc
    ${PHONON_LIBRARY}
    ${LIBVLCCORE_LIBRARY}
    ${LIBVLC_LIBRARY}
)

if(WIN32)
    install(TARGETS phonon_vlc DESTINATION bin/phonon_backend)
else(WIN32)
    install(TARGETS phonon_vlc DESTINATION ${PLUGIN_INSTALL_DIR}/plugins/phonon_backend)
endif(WIN32)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/utils/mime.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/utils/mime.h @ONLY)

# Instead of desktop files we are embedding the information into the plugin itself.
# We have no KDE technology to help with finding the actual libraries anyway, so
# we need to have the library path anyway.
# Also see qtplugin/Q_PLUGIN_METADATA documentation.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/phonon-vlc.json.in
               ${CMAKE_CURRENT_BINARY_DIR}/phonon-vlc.json @ONLY)
