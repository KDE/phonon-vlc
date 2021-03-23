/*
    Copyright (C) 2012-2021 Harald Sitter <sitter@kde.org>

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

#include <vlc/plugins/vlc_picture.h>

#include "mediaplayer.h"
#include "utils/debug.h"

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

unsigned VideoMemoryStream::setPitchAndLines(uint32_t fourcc,
                                             unsigned width, unsigned height,
                                             unsigned *pitches, unsigned *lines)
{
    // Fairly unclear what the last two arguments do, they seem to make no diff for the planes though, so I guess they can be anything in our case.
    const auto picture = picture_New(fourcc, width, height, 0, 1);

    unsigned bufferSize = 0;

    for (auto i = 0; i < picture->i_planes; ++i) {
        const auto plane = picture->p[i];
        pitches[i] = plane.i_visible_pitch;
        lines[i] = plane.i_visible_lines;
        bufferSize += (pitches[i] * lines[i]);
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
    auto ret = P_THIS->formatCallback(chroma, width, height, pitches, lines);

    if (Debug::debugEnabled()) {
        QStringList pitchValues;
        QStringList lineValues;
        unsigned *pitch = pitches;
        unsigned *line = lines;
        for (; *pitch != 0; ++pitch) {
            Q_ASSERT(lines != 0); // pitch and line tables ought to be the same size
            pitchValues << QString::number(*pitch);
            lineValues << QString::number(*line);
        }
        const QString separator = QStringLiteral(", ");
        debug() << "vmem-format[chroma:" << chroma << "w:" << *width << "h:" << *height
                << "pitches:" << pitchValues.join(separator) << "lines:" << lineValues.join(separator)
                << "size:" << ret << "]";
    }

    return ret;
}

void VideoMemoryStream::formatCleanUpCallbackInternal(void *opaque)
{
    P_THIS->formatCleanUpCallback();
}

} // namespace VLC
} // namespace Phonon
