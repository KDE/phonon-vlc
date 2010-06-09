/*  This file is part of the KDE project.

Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).

This library is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 or 3 of the License.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "v4l2devices.h"

#include <linux/videodev2.h>
#include <libv4l2.h>
#include <fcntl.h>

#define V4L2_DEFAULT "/dev/video0"

QT_BEGIN_NAMESPACE

namespace Phonon {
namespace VLC {
namespace V4L2Support {


typedef enum {
    IO_METHOD_AUTO,
    IO_METHOD_READ,
    IO_METHOD_MMAP,
    IO_METHOD_USERPTR,
} io_method;

struct demux_sys_t
{
    char *psz_device;  /* Main device from MRL */
    int  i_fd;

    char *psz_requested_chroma;

    /* Video */
    io_method io;

    int i_cache;

    struct v4l2_capability dev_cap;

    int i_input;
    struct v4l2_input *p_inputs;
    int i_selected_input;

    int i_standard;
    struct v4l2_standard *p_standards;
    v4l2_std_id i_selected_standard_id;

    int i_audio;
    /* V4L2 devices cannot have more than 32 audio inputs */
    struct v4l2_audio p_audios[32];
    int i_selected_audio_input;

    int i_tuner;
    struct v4l2_tuner *p_tuners;

    int i_codec;
    struct v4l2_fmtdesc *p_codecs;

    struct buffer_t *p_buffers;
    unsigned int i_nbuffers;

    int i_width;
    int i_height;
    unsigned int i_aspect;
    float f_fps;            /* <= 0.0 mean to grab at full rate */
    int64_t i_video_pts;    /* only used when f_fps > 0 */
    int i_fourcc;
    uint32_t i_block_flags;

    /* es_out_id_t *p_es; */
    void *p_es;

    /* Tuner */
    int i_cur_tuner;
    int i_frequency;
    int i_audio_mode;

    /* Controls */
    char *psz_set_ctrls;

    /* */
    int (*pf_open)(const char *, int, ...);
    int (*pf_close)( int );
    int (*pf_dup)( int );
    int (*pf_ioctl)( int, unsigned long int, ... );
    ssize_t (*pf_read)( int, void *, size_t );
    void *(*pf_mmap)( void *, size_t, int, int, int, off_t );
    int (*pf_munmap)( void *, size_t );
    bool b_libv4l2;
};


static bool probeVideoDev( demux_sys_t *p_sys, const char *psz_device )
{
    int i_index;
    int i_standard;

    int i_fd;

    if( ( i_fd = v4l2_open( psz_device, O_RDWR ) ) < 0 )
    {
        qDebug() << "cannot open video device" << psz_device;
        goto open_failed;
    }

    /* Note the v4l2_xxx functions are designed so that if they get passed an
    unknown fd, the will behave exactly as their regular xxx counterparts,
    so if v4l2_fd_open fails, we continue as normal (missing the libv4l2
    custom cam format to normal formats conversion). Chances are big we will
    still fail then though, as normally v4l2_fd_open only fails if the
    device is not a v4l2 device. */
    if( p_sys->b_libv4l2 )
    {
        int libv4l2_fd;
        libv4l2_fd = v4l2_fd_open( i_fd, V4L2_ENABLE_ENUM_FMT_EMULATION );
        if( libv4l2_fd != -1 )
            i_fd = libv4l2_fd;
    }

    /* Get device capabilites */
    if( v4l2_ioctl( i_fd, VIDIOC_QUERYCAP, &p_sys->dev_cap ) < 0 )
    {
        qDebug() << "cannot get video capabilities";
        goto open_failed;
    }

    /* Now, enumerate all the video inputs. This is useless at the moment
    since we have no way to present that info to the user except with
    debug messages */

    if( p_sys->dev_cap.capabilities & V4L2_CAP_VIDEO_CAPTURE )
    {
        struct v4l2_input t_input;
        memset( &t_input, 0, sizeof(t_input) );
        p_sys->i_input = 0;
        while( v4l2_ioctl( i_fd, VIDIOC_ENUMINPUT, &t_input ) >= 0 )
        {
            p_sys->i_input++;
            t_input.index = p_sys->i_input;
        }

        free( p_sys->p_inputs );
        p_sys->p_inputs = (v4l2_input*) calloc( 1, p_sys->i_input * sizeof( struct v4l2_input ) );
        if( !p_sys->p_inputs ) goto open_failed;

        for( i_index = 0; i_index < p_sys->i_input; i_index++ )
        {
            p_sys->p_inputs[i_index].index = i_index;

            if( v4l2_ioctl( i_fd, VIDIOC_ENUMINPUT, &p_sys->p_inputs[i_index] ) )
            {
                qDebug() << "cannot get video input characteristics";
                goto open_failed;
            }
        }
    }

    /* initialize the structures for the ioctls */
    for( i_index = 0; i_index < 32; i_index++ )
    {
        p_sys->p_audios[i_index].index = i_index;
    }

    /* Probe audio inputs */
    if( p_sys->dev_cap.capabilities & V4L2_CAP_AUDIO )
    {
        while( p_sys->i_audio < 32 &&
            v4l2_ioctl( i_fd, VIDIOC_S_AUDIO, &p_sys->p_audios[p_sys->i_audio] ) >= 0 )
        {
            if( v4l2_ioctl( i_fd, VIDIOC_G_AUDIO, &p_sys->p_audios[ p_sys->i_audio] ) < 0 )
            {
                qDebug() << "cannot get audio input characteristics";
                goto open_failed;
            }

            p_sys->i_audio++;
        }
    }

    if( i_fd >= 0 )
        v4l2_close( i_fd );
    return true;

    open_failed:

    if( i_fd >= 0 )
        v4l2_close( i_fd );
    return false;

}

}
}
} // namespace Phonon::VLC::V4L2Support

QT_END_NAMESPACE