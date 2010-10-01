/*****************************************************************************
 * libVLC backend for the Phonon library                                     *
 *                                                                           *
 * Copyright (C) 2006 Matthias Kretz <kretz@kde.org>                         *
 * Copyright (C) 2009 Martin Sandsmark <sandsmark@samfundet.no>              *
 * Copyright (C) 2010 Ben Cooksley <sourtooth@gmail.com>                     *
 * Copyright (C) 2010 Harald Sitter <apachelogger@ubuntu.com>                *
 *                                                                           *
 * This program is free software; you can redistribute it and/or             *
 * modify it under the terms of the GNU Lesser General Public                *
 * License as published by the Free Software Foundation; either              *
 * version 2.1 of the License, or (at your option) any later version.        *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 * Lesser General Public License for more details.                           *
 *                                                                           *
 * You should have received a copy of the GNU Lesser General Public          *
 * License along with this package; if not, write to the Free Software       *
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA *
 *****************************************************************************/

#ifndef Phonon_VLC_VIDEODATAOUTPUT_H
#define Phonon_VLC_VIDEODATAOUTPUT_H

#include "backend.h"
#include "sinknode.h"
#include <phonon/experimental/videodataoutputinterface.h>

#include <QtCore/QMutex>

// Allow PRId64 to be defined:
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

namespace Phonon
{
namespace VLC
{
/**
 * @author Martin Sandsmark <sandsmark@samfundet.no>
 * @author Harald Sitter <apachelogger@ubuntu.com>
 */
class VideoDataOutput : public SinkNode,
    public Experimental::VideoDataOutputInterface
{
    Q_OBJECT
    Q_INTERFACES(Phonon::Experimental::VideoDataOutputInterface)

public:
    /**
     * Constructor.
     *
     * @param parent parenting object
     */
    VideoDataOutput(QObject *parent);

    /**
     * Destructor.
     */
    ~VideoDataOutput();

    /**
      TODO
     */
    void connectToMediaObject(PrivateMediaObject *mediaObject);

    /**
     * Overloaded from VideoDataOutputInterface.
     *
     * @return frontendObject with which the interface interacts with the public
     *
     * @see setFrontendObject()
     */
    Experimental::AbstractVideoDataOutput *frontendObject() const;

    /**
     * Overloaded from VideoDataOutputInterface.
     *
     * @param frontend The frontent the interface is supposed to use to interact
     *        with the public
     *
     * @see frontendObject()
     */
    void setFrontendObject(Experimental::AbstractVideoDataOutput *frontend);

    /**
     * Call back function for libVLC.
     *
     * This function is public so that the compiler does not fall over.
     *
     * This function gets called by libVLC to lock the context, i.e. this
     * interface.
     *
     * @param data pointer to 'this'
     * @param buffer when this function returns this pointer should contain the
     *        address of a buffer to use for libVLC
     *
     * @return picture identifier - NOT USED -> ALWAYS NULL
     *
     * @see unlock()
     */
    static void *lock(void *data, void **buffer);

    /**
     * Call back function for libVLC.
     *
     * This function is public so that the compiler does not fall over.
     *
     * @param data pointer to 'this'
     * @param id TODO: dont know that off the top of my head
     * @param pixels the pixel buffer of the current frame
     *
     * @see lock()
     */
    static void unlock(void *data, void *id, void *const *pixels);

public Q_SLOTS:
    /**
     * Sets the format and callbacks.
     *
     * @param mediaObject unused right now
     * FIXME
     */
    void addToMedia(libvlc_media_t *media);

private Q_SLOTS:
    /**
     * This slot is called when the internal video size needs to be changed.
     * The internal QImage will be replaced with a new appropriately sized one.
     *
     * @param width new width of the video
     * @param height new height of the video
     */
    void videoSizeChanged(int width, int height);

private:
    Experimental::AbstractVideoDataOutput *m_frontend;

    QMutex m_mutex;
    QImage *m_img;
};
}
} //namespace Phonon::VLC

QT_END_NAMESPACE
QT_END_HEADER

// vim: sw=4 ts=4 tw=80
#endif // Phonon_VLC_VIDEODATAOUTPUT_H
