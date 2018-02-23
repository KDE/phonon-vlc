# Copyright (C) 2012, Harald Sitter <sitter@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

set(PHONON_VLC_MIME_TYPES
    application/mpeg4-iod
    application/mpeg4-muxcodetable
    application/mxf
    application/ogg
    application/ram
    application/sdp
    application/vnd.apple.mpegurl
    application/vnd.ms-asf
    application/vnd.ms-wpl
    application/vnd.rn-realmedia
    application/vnd.rn-realmedia-vbr
    application/x-cd-image
    application/x-extension-m4a
    application/x-extension-mp4
    application/x-flac
    application/x-flash-video
    application/x-matroska
    application/x-ogg
    application/x-quicktime-media-link
    application/x-quicktimeplayer
    application/x-shockwave-flash
    application/xspf+xml
    audio/3gpp
    audio/3gpp2
    audio/AMR
    audio/AMR-WB
    audio/aac
    audio/ac3
    audio/basic
    audio/dv
    audio/eac3
    audio/flac
    audio/m4a
    audio/midi
    audio/mp1
    audio/mp2
    audio/mp3
    audio/mp4
    audio/mpeg
    audio/mpegurl
    audio/mpg
    audio/ogg
    audio/opus
    audio/scpls
    audio/vnd.dolby.heaac.1
    audio/vnd.dolby.heaac.2
    audio/vnd.dolby.mlp
    audio/vnd.dts
    audio/vnd.dts.hd
    audio/vnd.rn-realaudio
    audio/vorbis
    audio/wav
    audio/webm
    audio/x-aac
    audio/x-adpcm
    audio/x-aiff
    audio/x-ape
    audio/x-flac
    audio/x-gsm
    audio/x-it
    audio/x-m4a
    audio/x-matroska
    audio/x-mod
    audio/x-mp1
    audio/x-mp2
    audio/x-mp3
    audio/x-mpeg
    audio/x-mpegurl
    audio/x-mpg
    audio/x-ms-asf
    audio/x-ms-asx
    audio/x-ms-wax
    audio/x-ms-wma
    audio/x-musepack
    audio/x-pn-aiff
    audio/x-pn-au
    audio/x-pn-realaudio
    audio/x-pn-realaudio-plugin
    audio/x-pn-wav
    audio/x-pn-windows-acm
    audio/x-real-audio
    audio/x-realaudio
    audio/x-s3m
    audio/x-scpls
    audio/x-shorten
    audio/x-speex
    audio/x-tta
    audio/x-vorbis
    audio/x-vorbis+ogg
    audio/x-wav
    audio/x-wavpack
    audio/x-xm
    image/vnd.rn-realpix
    misc/ultravox
    text/google-video-pointer
    text/x-google-video-pointer
    video/3gp
    video/3gpp
    video/3gpp2
    video/avi
    video/divx
    video/dv
    video/fli
    video/flv
    video/mp2t
    video/mp4
    video/mp4v-es
    video/mpeg
    video/mpeg-system
    video/msvideo
    video/ogg
    video/quicktime
    video/vnd.divx
    video/vnd.mpegurl
    video/vnd.rn-realvideo
    video/webm
    video/x-anim
    video/x-avi
    video/x-flc
    video/x-fli
    video/x-flv
    video/x-m4v
    video/x-matroska
    video/x-mpeg
    video/x-mpeg-system
    video/x-mpeg2
    video/x-ms-asf
    video/x-ms-asf-plugin
    video/x-ms-asx
    video/x-ms-wm
    video/x-ms-wmv
    video/x-ms-wmx
    video/x-ms-wvx
    video/x-msvideo
    video/x-nsv
    video/x-ogm
    video/x-ogm+ogg
    video/x-theora
    video/x-theora+ogg
    x-content/audio-cdda
    x-content/audio-player
    x-content/video-dvd
    x-content/video-svcd
    x-content/video-vcd
)

macro(CREATE_C_ARRAY var list)
    set(ret "")
    foreach(str ${PHONON_VLC_MIME_TYPES})
        if(NOT ret)
            set(ret "\"${str}\"")
        else(NOT ret)
            set(ret "${ret}, \"${str}\"")
        endif(NOT ret)
    endforeach(str)
    set(${var} "{${ret}, 0}")
endmacro(CREATE_C_ARRAY var list)

CREATE_C_ARRAY(PHONON_VLC_MIME_TYPES_C_ARRAY ${PHONON_VLC_MIME_TYPES})
