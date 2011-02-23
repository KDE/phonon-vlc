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
                message (FATAL_ERROR  "You don't have stdint.h and inttypes.h\n\t get them from  http://code.google.com/p/baseutils/source/browse/#svn/trunk/msvc,\n\t or get kdewin http://websvn.kde.org/trunk/kdesupport/kdewin/")
            endif ( NOT HAVE_STDINT AND NOT HAVE_INTTYPES )
        endif (KDEWIN_FOUND)
endif (MSVC)

# Try and find libv4l2
if (CMAKE_SYSTEM_NAME MATCHES Linux)
    macro_optional_find_package(V4L2)
    macro_log_feature(V4L2_FOUND "V4L2" "VideoForLinux 2 is required to enable listing v4l2 devices" "" FALSE)
    if (V4L2_FOUND)
        add_definitions(-DHAVE_LIBV4L2)
        include_directories(${LIBV4L2_INCLUDE_DIR})
    endif (V4L2_FOUND)
endif (CMAKE_SYSTEM_NAME MATCHES Linux)

add_definitions(-D_PHONON_BACKEND_VERSION_4_2)

option(PHONON_VLC_NO_EXPERIMENTAL "Do not build anything experimental" true)
if(PHONON_VLC_NO_EXPERIMENTAL)
    add_definitions(-DPHONON_VLC_NO_EXPERIMENTAL)
else(PHONON_VLC_NO_EXPERIMENTAL)
    add_subdirectory(experimental)
endif(PHONON_VLC_NO_EXPERIMENTAL)

set(phonon_vlc_SRCS
    audiooutput.cpp
    audiodataoutput.cpp
    backend.cpp
    devicemanager.cpp
    effect.cpp
    effectmanager.cpp
    libvlc.cpp
    mediacontroller.cpp
    mediaobject.cpp
    overlaywidget.cpp
    seekstack.cpp
    sinknode.cpp
    streamhooks.cpp
    streamreader.cpp
    #    videodataoutput.cpp
    devicescan.cpp
    videowidget.cpp
    ${phonon_vlc_experimental_SRCS}
)

if(APPLE)
set(phonon_vlc_mac_SRCS
     nsvideoview.mm
     vlcmacwidget.mm
     vlcvideoview.mm)
endif(APPLE)

automoc4_add_library(phonon_vlc MODULE ${phonon_vlc_SRCS} )

set_target_properties(phonon_vlc PROPERTIES PREFIX "")
target_link_libraries(phonon_vlc ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${PHONON_LIBRARY} ${LIBVLC_LIBRARY} ${LIBVLCCORE_LIBRARY})

if(QT_QTDBUS_FOUND)
    target_link_libraries(phonon_vlc ${QT_QTDBUS_LIBRARY})
endif(QT_QTDBUS_FOUND)

if (V4L2_FOUND)
    target_link_libraries(phonon_vlc ${V4L2_LIBRARY})
endif (V4L2_FOUND)

if(WIN32)
    install(TARGETS phonon_vlc DESTINATION bin/phonon_backend)
else(WIN32)
    install(TARGETS phonon_vlc DESTINATION ${PLUGIN_INSTALL_DIR}/plugins/phonon_backend)
endif(WIN32)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vlc.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/vlc.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vlc.desktop DESTINATION ${SERVICES_INSTALL_DIR}/phononbackends)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif(BUILD_TESTS)
