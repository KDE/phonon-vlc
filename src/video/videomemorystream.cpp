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

unsigned VideoMemoryStream::setPitchAndLines(const vlc_chroma_description_t *desc,
                                             unsigned width, unsigned height,
                                             unsigned *pitches, unsigned *lines)
{
    unsigned int bufferSize = 0;
    for (unsigned int i = 0; i < desc->plane_count; ++i) {
        pitches[i] = width * desc->p[i].w.num / desc->p[i].w.den * desc->pixel_size;
        lines[i] = height * desc->p[i].h.num / desc->p[i].h.den;

        bufferSize += pitches[i] * lines[i];
    }
    return bufferSize;
}

void VideoMemoryStream::setCallbacks(MediaPlayer *player)
{
#warning needs wrappers
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
