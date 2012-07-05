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

#ifndef Phonon_VLC_VIDEODATAOUTPUT_H
#define Phonon_VLC_VIDEODATAOUTPUT_H

#include <QMutex>
#include <QObject>

#include <phonon/experimental/videodataoutputinterface.h>
#include <phonon/experimental/videoframe2.h>

#include "sinknode.h"
#include "videomemorystream.h"

namespace Phonon
{
namespace VLC
{

/**
 * @author Harald Sitter <apachelogger@ubuntu.com>
 */
class VideoDataOutput : public QObject, public SinkNode,
        public Experimental::VideoDataOutputInterface, private VideoMemoryStream
{
    Q_OBJECT
    Q_INTERFACES(Phonon::Experimental::VideoDataOutputInterface)
public:
    explicit VideoDataOutput(QObject *parent);
    ~VideoDataOutput();

    void connectToMediaObject(MediaObject *mediaObject);
    void disconnectFromMediaObject(MediaObject *mediaObject);

    Experimental::AbstractVideoDataOutput *frontendObject() const;
    void setFrontendObject(Experimental::AbstractVideoDataOutput *frontend);

    virtual void *lockCallback(void **planes);
    virtual void unlockCallback(void *picture,void *const *planes);
    virtual void displayCallback(void *picture);

    virtual unsigned formatCallback(char *chroma,
                                    unsigned *width, unsigned *height,
                                    unsigned *pitches,
                                    unsigned *lines);
    virtual void formatCleanUpCallback();

private:
    Experimental::AbstractVideoDataOutput *m_frontend;
    Experimental::VideoFrame2 m_frame;
    QByteArray m_buffer;
    QMutex m_mutex;
};

} // namespace VLC
} // namespace Phonon

#endif // PHONON_VLC_VIDEODATAOUTPUT_H
