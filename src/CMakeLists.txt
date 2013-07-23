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

add_definitions(-D_PHONON_BACKEND_VERSION_4_2)

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
)

if(NOT PHONON_NO_GRAPHICSVIEW)
    list(APPEND phonon_vlc_SRCS video/videographicsobject.cpp)
else()
    add_definitions(-DPHONON_NO_GRAPHICSVIEW)
endif()

if(PHONON_FOUND_EXPERIMENTAL)
    add_definitions(-DPHONON_EXPERIMENTAL)
    list(APPEND phonon_vlc_SRCS video/videodataoutput.cpp)
endif(PHONON_FOUND_EXPERIMENTAL)

if(APPLE)
    list(APPEND phonon_vlc_SRCS
        video/mac/nsvideoview.mm
        video/mac/vlcmacwidget.mm)
endif(APPLE)

automoc4_add_library(phonon_vlc MODULE ${phonon_vlc_SRCS})
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

if(PHONON_BUILD_PHONON4QT5)
    # Instead of desktop files we are embedding the information into the plugin itself.
    # We have no KDE technology to help with finding the actual libraries anyway, so
    # we need to have the library path anyway.
    # Also see qtplugin/Q_PLUGIN_METADATA documentation.
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/phonon-vlc.json.in
                   ${CMAKE_CURRENT_BINARY_DIR}/phonon-vlc.json @ONLY)
else()
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vlc.desktop.cmake
                   ${CMAKE_CURRENT_BINARY_DIR}/vlc.desktop @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vlc.desktop
            DESTINATION ${SERVICES_INSTALL_DIR}/phononbackends)
endif()

