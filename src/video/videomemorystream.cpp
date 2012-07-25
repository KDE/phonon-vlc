/*
    Copyright (C) 2012 Harald Sitter <sitter@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "videomemorystream.h"

#include "mediaplayer.h"
//#include "utils/qDebug.h"
#include <QDebug>

namespace Phonon {
namespace VLC {

static inline VideoMemoryStream *p_this(void *opaque) { return static_cast<VideoMemoryStream *>(opaque); }
static inline VideoMemoryStream *p_this(void **opaque) { return static_cast<VideoMemoryStream *>(*opaque); }
#define P_THIS p_this(opaque)

VideoMemoryStream::VideoMemoryStream()
{
}

VideoMemoryStream::~VideoMemoryStream()
{
}

static inline qint64 gcd(qint64 a, qint64 b)
{
    while( b )
    {
        qint64 c = a % b;
        a = b;
        b = c;
    }
    return a;
}

static int lcm(int a, int b)
{
    return a * b / GCD( a, b );
}


unsigned VideoMemoryStream::setPitchAndLines(const vlc_chroma_description_t *desc,
                                             unsigned width, unsigned height,
                                             unsigned *pitches, unsigned *lines,
                                             unsigned *visiblePitches, unsigned *visibleLines)
{
    // Mostly taken from vlc/src/misc/picture.c
    // Simple alignment would be an option but I trust the VLC guys they know what they are doing.
    int i_modulo_w = 1;
    int i_modulo_h = 1;
    unsigned int i_ratio_h  = 1;
    for( unsigned i = 0; i < desc->plane_count; i++ )
    {
        i_modulo_w = lcm( i_modulo_w, 8 * desc->p[i].w.den );
        i_modulo_h = lcm( i_modulo_h, 8 * desc->p[i].h.den );
        if( i_ratio_h < desc->p[i].h.den )
            i_ratio_h = desc->p[i].h.den;

    }
    i_modulo_h = lcm( i_modulo_h, 32 );

    const int i_width_aligned  = ( width  + i_modulo_w - 1 ) / i_modulo_w * i_modulo_w;
    const int i_height_aligned = ( height + i_modulo_h - 1 ) / i_modulo_h * i_modulo_h;
    const int i_height_extra   = 2 * i_ratio_h; /* This one is a hack for some ASM functions */

    unsigned int bufferSize = 0;
    for(unsigned i = 0; i < desc->plane_count; ++i)
    {
        //    qDebug() << width << "x" << height;
        //    qDebug() << (i_height_aligned + i_height_extra ) * desc->p[i].h.num / desc->p[i].h.den << "/" << height * desc->p[i].h.num / desc->p[i].h.den;
        //    qDebug() << i_width_aligned * desc->p[i].w.num / desc->p[i].w.den * desc->pixel_size << "/" << (width * desc->p[i].w.num / desc->p[i].w.den * desc->pixel_size);
        //    qDebug() << QString("%1 * %2 / %3 * %4").arg(QString::number(width), QString::number(desc->p[i].w.num), QString::number(desc->p[i].w.den), QString::number(desc->pixel_size));
        pitches[i] = i_width_aligned * desc->p[i].w.num / desc->p[i].w.den * desc->pixel_size;
        if (visiblePitches)
            visiblePitches[i] = width * desc->p[i].w.num / desc->p[i].w.den * desc->pixel_size;

        lines[i] = (i_height_aligned + i_height_extra ) * desc->p[i].h.num / desc->p[i].h.den;
        if (visibleLines)
            visibleLines[i] = height * desc->p[i].h.num / desc->p[i].h.den;

        bufferSize += pitches[i] * lines[i];
    }

    return bufferSize;
}

void VideoMemoryStream::setCallbacks(MediaPlayer *player)
{
    libvlc_video_set_callbacks(player->libvlc_media_player(),
                               lockCallbackInternal,
                               unlockCallbackInternal,
                               displayCallbackInternal,
                               this);
    libvlc_video_set_format_callbacks(player->libvlc_media_player(),
                                      formatCallbackInternal,
                                      formatCleanUpCallbackInternal);
}

void VideoMemoryStream::unsetCallbacks(MediaPlayer *player)
{
    libvlc_video_set_callbacks(player->libvlc_media_player(),
                               0,
                               0,
                               0,
                               0);
    libvlc_video_set_format_callbacks(player->libvlc_media_player(),
                                      0,
                                      0);
}


void *VideoMemoryStream::lockCallbackInternal(void *opaque, void **planes)
{
    return P_THIS->lockCallback(planes);
}

void VideoMemoryStream::unlockCallbackInternal(void *opaque, void *picture, void *const*planes)
{
    P_THIS->unlockCallback(picture, planes);
}

void VideoMemoryStream::displayCallbackInternal(void *opaque, void *picture)
{
    P_THIS->displayCallback(picture);
}

unsigned VideoMemoryStream::formatCallbackInternal(void **opaque, char *chroma,
                                                   unsigned *width, unsigned *height,
                                                   unsigned *pitches, unsigned *lines)
{
    return P_THIS->formatCallback(chroma, width, height, pitches, lines);
}

void VideoMemoryStream::formatCleanUpCallbackInternal(void *opaque)
{
    P_THIS->formatCleanUpCallback();
}

} // namespace VLC
} // namespace Phonon
