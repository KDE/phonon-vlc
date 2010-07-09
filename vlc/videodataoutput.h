/*****************************************************************************
 * libVLC backend for the Phonon library                                     *
 *                                                                           *
 * Copyright (C) 2006 Matthias Kretz <kretz@kde.org>
 * Copyright (C) 2009 Martin Sandsmark <sandsmark@samfundet.no>
 * Copyright (C) 2010 Ben Cooksley <sourtooth@gmail.com>
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
#include <QtCore/QMutex>
#include <phonon/experimental/videodataoutput2.h>
#include <phonon/experimental/videodataoutputinterface.h>

//Allow PRId64 to be defined:
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

namespace Phonon
{
namespace VLC
{
    /**
     * \author Martin Sandsmark <sandsmark@samfundet.no>
     */
    class VideoDataOutput : public SinkNode,
                            public Experimental::VideoDataOutputInterface
    {
        Q_OBJECT
        Q_INTERFACES(Phonon::Experimental::VideoDataOutputInterface)

        public:
            VideoDataOutput(Backend *, QObject *);
            ~VideoDataOutput();

            Experimental::AbstractVideoDataOutput *frontendObject() const;
            void setFrontendObject(Experimental::AbstractVideoDataOutput *frontend);

        Q_SIGNALS:
            void frameReadySignal(const Experimental::VideoFrame2 &frame);

        public Q_SLOTS:
            void addToMedia(libvlc_media_t * media);

        private:
            static void lock(VideoDataOutput *cw, void **bufRet);
            static void unlock(VideoDataOutput *cw);

            int m_dataSize;
            int m_sampleRate;
//            Phonon::Experimental::VideoDataOutput2 *m_frontend;

            QMutex m_locker;
            char * m_buffer;

            Experimental::AbstractVideoDataOutput *m_frontend;
    };
}} //namespace Phonon::VLC

QT_END_NAMESPACE
QT_END_HEADER

// vim: sw=4 ts=4 tw=80
#endif // Phonon_VLC_VIDEODATAOUTPUT_H
