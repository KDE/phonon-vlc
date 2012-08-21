set(PHONON_VLC_MIME_TYPES
    application/ogg
    application/x-ogg
    application/vnd.rn-realmedia
    application/x-annodex
    application/x-flash-video
    application/x-quicktimeplayer
    application/x-extension-mp4
    audio/168sv
    audio/3gpp
    audio/3gpp2
    audio/8svx
    audio/aiff
    audio/amr
    audio/amr-wb
    audio/basic
    audio/mp3
    audio/mp4
    audio/midi
    audio/mpeg
    audio/mpeg2
    audio/mpeg3
    audio/vnd.rn-realaudio
    audio/vnd.rn-realmedia
    audio/wav
    audio/webm
    audio/x-16sv
    audio/x-8svx
    audio/x-aiff
    audio/x-basic
    audio/x-it
    audio/x-m4a
    audio/x-matroska
    audio/x-mp3
    audio/x-mpeg
    audio/x-mpeg2
    audio/x-mpeg3
    audio/x-mpegurl
    audio/x-ms-wma
    audio/x-ogg
    audio/x-pn-aiff
    audio/x-pn-au
    audio/x-pn-realaudio-plugin
    audio/x-pn-wav
    audio/x-pn-windows-acm
    audio/x-real-audio
    audio/x-realaudio
    audio/x-s3m
    audio/x-speex+ogg
    audio/x-vorbis+ogg
    audio/x-wav
    audio/x-xm
    image/ilbm
    image/png
    image/x-ilbm
    image/x-png
    video/3gpp
    video/3gpp2
    video/anim
    video/avi
    video/divx
    video/flv
    video/mkv
    video/mng
    video/mp4
    video/mpeg
    video/mpeg-system
    video/mpg
    video/msvideo
    video/ogg
    video/quicktime
    video/webm
    video/x-anim
    video/x-flic
    video/x-flv
    video/x-matroska
    video/x-mng
    video/x-m4v
    video/x-mpeg
    video/x-mpeg-system
    video/x-ms-asf
    video/x-ms-wma
    video/x-ms-wmv
    video/x-ms-wvx
    video/x-msvideo
    video/x-quicktime
    audio/x-flac
    audio/x-ape
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
message("${PHONON_VLC_MIME_TYPES_C_ARRAY}")