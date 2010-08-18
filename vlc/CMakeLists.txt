project(phonon-vlc)
include_directories(${QT_INCLUDES}
                    ${LIBVLC_INCLUDE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR})

if (MSVC)
    set (CMAKE_MODULE_PATH ${CMAKE_INSTALL_PREFIX}/share/apps/cmake/modules)
    find_package(KDEWIN)
        if (KDEWIN_FOUND)
            include_directories(${KDEWIN_INCLUDES}/msvc)
            link_Phononlibraries(${KDEWIN_LIBRARIES})
        else (KDEWIN_FOUND)
            include(CheckIncludeFileCXX)
            check_include_file_cxx(inttypes.h HAVE_INTTYPES)
            check_include_file_cxx(stdint.h   HAVE_STDINT)
            if ( NOT HAVE_STDINT AND NOT HAVE_INTTYPES )
                message (FATAL_ERROR  "You don't have stdint.h and inttypes.h\n\t get them from  http://code.google.com/p/baseutils/source/browse/#svn/trunk/msvc,\n\t or get kdewin http://websvn.kde.org/trunk/kdesupport/kdewin/")
            endif ( NOT HAVE_STDINT AND NOT HAVE_INTTYPES )
        endif (KDEWIN_FOUND)
endif (MSVC)

add_definitions(-D_PHONON_BACKEND_VERSION_4_2)

set(phonon_vlc_SRCS
    audiooutput.cpp
    audiodataoutput.cpp
    backend.cpp
    devicemanager.cpp
    effect.cpp
    effectmanager.cpp
    mediacontroller.cpp
    mediaobject.cpp
    seekstack.cpp
    sinknode.cpp
    streamhooks.cpp
    streamreader.cpp
    #    videodataoutput.cpp
    videowidget.cpp
    vlcloader.cpp
    vlcmediacontroller.cpp
    vlcmediaobject.cpp
    vlcvideowidget.cpp
    widgetnopaintevent.cpp)

automoc4(phonon_vlc phonon_vlc_SRCS)

if(APPLE)
set(phonon_vlc_mac_SRCS
     nsvideoview.mm
     vlcmacwidget.mm
     vlcvideoview.mm)
endif(APPLE)

add_library(phonon_vlc MODULE ${phonon_vlc_SRCS} ${phonon_vlc_mac_SRCS})

set_target_properties(phonon_vlc PROPERTIES PREFIX "")
target_link_libraries(phonon_vlc ${PHONON_LIBS} ${LIBVLC_LIBRARY}
    ${LIBVLCCORE_LIBRARY} )

if(WIN32)
    install(TARGETS phonon_vlc DESTINATION bin/phonon_backend)
else(WIN32)
    install(TARGETS phonon_vlc DESTINATION ${PLUGIN_INSTALL_DIR}/plugins/phonon_backend)
endif(WIN32)
install(FILES vlc.desktop DESTINATION ${SERVICES_INSTALL_DIR}/phononbackends)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif(BUILD_TESTS)
