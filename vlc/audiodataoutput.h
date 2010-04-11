/*  This file is part of the KDE project
    Copyright (C) 2006 Matthias Kretz <kretz@kde.org>
    Copyright (C) 2009 Martin Sandsmark <sandsmark@samfundet.no>
    Copyright (C) 2010 Ben Cooksley <sourtooth@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), Nokia Corporation
    (or its successors, if any) and the KDE Free Qt Foundation, which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef Phonon_VLC_AUDIODATAOUTPUT_H
#define Phonon_VLC_AUDIODATAOUTPUT_H

#include "backend.h"
#include "sinknode.h"
#include <QtCore/QMutex>
#include <phonon/audiodataoutput.h>
#include <phonon/audiodataoutputinterface.h>

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
    class AudioDataOutput : public SinkNode,
                            public AudioDataOutputInterface
    {
        Q_OBJECT
        Q_INTERFACES(Phonon::AudioDataOutputInterface)

        public:
            AudioDataOutput(Backend *, QObject *);
            ~AudioDataOutput();

        public Q_SLOTS:
            int dataSize() const;
            int sampleRate() const;
            void setDataSize(int size);
            void addToMedia( libvlc_media_t * media );

        public:
            Phonon::AudioDataOutput* frontendObject() const { return m_frontend; }
            void setFrontendObject(Phonon::AudioDataOutput *frontend) { m_frontend = frontend; }

        signals:
            void dataReady(const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > &data);
            void dataReady(const QMap<Phonon::AudioDataOutput::Channel, QVector<float> > &data);
            void endOfMedia(int remainingSamples);
            void sampleReadDone();

        private Q_SLOTS:
            void sendData();

        private:
            static void lock( AudioDataOutput *cw, quint8 **pcm_buffer , quint32 size );
            static void unlock( AudioDataOutput *cw, quint8 *pcm_buffer,
                                quint32 channels, quint32 rate,
                                quint32 nb_samples, quint32 bits_per_sample,
                                quint32 size, qint64 pts );

            int m_dataSize;
            int m_sampleRate;
            Phonon::AudioDataOutput *m_frontend;

            QMutex m_locker;
            int m_channel_count;
            QVector<qint16> m_channel_samples[6];
            QList<Phonon::AudioDataOutput::Channel> m_channels;
    };
}} //namespace Phonon::VLC

QT_END_NAMESPACE
QT_END_HEADER

// vim: sw=4 ts=4 tw=80
#endif // Phonon_VLC_AUDIODATAOUTPUT_H
