
include_directories(${PHONON_EXPERIMENTAL_INCLUDE_DIR})

set(phonon_vlc_experimental_SRCS
    avcapture.cpp
)

add_library(phonon_vlc_experimental STATIC ${phonon_vlc_experimental_SRCS})

