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

#ifndef PHONON_VLC_VIDEOMEMORYSTREAM_H
#define PHONON_VLC_VIDEOMEMORYSTREAM_H

// VLC 3.0 uses the restrict keyword. restrict is not a thing in C++, so
// depending on the compiler you use an extension keyword or drop it entirely.
#if defined(Q_CC_GNU)
#define restrict __restrict__
#elif defined(Q_CC_MSVC)
#define restrict __restrict
#else
#define restrict
#endif

#include <vlc/plugins/vlc_common.h>
#include <vlc/plugins/vlc_fourcc.h>

namespace Phonon {
namespace VLC {

class MediaPlayer;

class VideoMemoryStream
{
public:
    explicit VideoMemoryStream();
    virtual ~VideoMemoryStream();

    /**
     * @returns overall buffersize needed
     */
    static unsigned setPitchAndLines(const vlc_chroma_description_t *chromaDescription,
                                     unsigned width, unsigned height,
                                     unsigned *pitches, unsigned *lines,
                                     unsigned *visiblePitches = 0, unsigned *visibleLines = 0);

    void setCallbacks(Phonon::VLC::MediaPlayer *player);
    void unsetCallbacks(Phonon::VLC::MediaPlayer *player);

protected:
    virtual void *lockCallback(void **planes) = 0;
    virtual void unlockCallback(void *picture,void *const *planes) = 0;
    virtual void displayCallback(void *picture) = 0;

    virtual unsigned formatCallback(char *chroma,
                                    unsigned *width, unsigned *height,
                                    unsigned *pitches,
                                    unsigned *lines) = 0;
    virtual void formatCleanUpCallback() = 0;

private:
    static void *lockCallbackInternal(void *opaque, void **planes);
    static void unlockCallbackInternal(void *opaque, void *picture, void *const *planes);
    static void displayCallbackInternal(void *opaque, void *picture);

    static unsigned formatCallbackInternal(void **opaque, char *chroma,
                                           unsigned *width, unsigned *height,
                                           unsigned *pitches,
                                           unsigned *lines);
    static void formatCleanUpCallbackInternal(void *opaque);

};

} // namespace VLC
} // namespace Phonon

#endif // PHONON_VLC_VIDEOMEMORYSTREAM_H
