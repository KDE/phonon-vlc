/*
    Copyright (C) 2011-2012 Harald Sitter <sitter@kde.org>

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

#ifndef PHONON_VLC_VIDEOGRAPHICSOBJECT_H
#define PHONON_VLC_VIDEOGRAPHICSOBJECT_H

#include <QtCore/QObject>
#include <QtCore/QMutex>

#include <phonon/videoframe.h>
#include <phonon/videographicsobjectinterface.h>

#include <vlc/libvlc_version.h>

#include "sinknode.h"
#include "videomemorystream.h"

struct libvlc_media_t;

namespace Phonon {
namespace VLC {

class VideoGraphicsObject : public QObject,
                            public VideoGraphicsObjectInterface,
                            public SinkNode,
                            public VideoMemoryStream
{
    Q_OBJECT
    Q_INTERFACES(Phonon::VideoGraphicsObjectInterface)
public:
    explicit VideoGraphicsObject(QObject *parent = 0);
    virtual ~VideoGraphicsObject();

    virtual void connectToMediaObject(MediaObject *mediaObject);
    virtual void disconnectFromMediaObject(MediaObject *mediaObject);

    void lock();
    bool tryLock();
    void unlock();

    const VideoFrame *frame() const { return &m_frame; }

    Q_INVOKABLE QList<VideoFrame::Format> offering(QList<VideoFrame::Format> offers);
    Q_INVOKABLE void choose(VideoFrame::Format format);

    virtual void *lockCallback(void **planes);
    virtual void unlockCallback(void *picture,void *const *planes);
    virtual void displayCallback(void *picture);

    virtual unsigned formatCallback(char *chroma,
                                    unsigned *width, unsigned *height,
                                    unsigned *pitches,
                                    unsigned *lines);
    virtual void formatCleanUpCallback();

signals:
    void frameReady();
    void reset();

    void needFormat();

protected:
    QMutex m_mutex;

    Phonon::VideoFrame m_frame;

    Phonon::VideoFrame::Format m_chosenFormat;
};

} // namespace VLC
} // namespace Phonon

#endif // PHONON_VLC_VIDEOGRAPHICSOBJECT_H
