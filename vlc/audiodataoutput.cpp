/*  This file is part of the KDE project
    Copyright (C) 2006 Matthias Kretz <kretz@kde.org>
    Copyright (C) 2009 Martin Sandsmark <sandsmark@samfundet.no>

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

#include "audiodataoutput.h"
#include "medianode.h"
#include "mediaobject.h"
#include <QtCore/QVector>
#include <QtCore/QMap>
#include <phonon/audiooutput.h>

namespace Phonon
{
namespace VLC
{
AudioDataOutput::AudioDataOutput(Backend *backend, QObject *parent)
    : SinkNode(parent)
{
}

AudioDataOutput::~AudioDataOutput()
{
}

int AudioDataOutput::dataSize() const
{
    return m_dataSize;
}

int AudioDataOutput::sampleRate() const
{
    return 44100;
}

void AudioDataOutput::setDataSize(int size)
{
    m_dataSize = size;
}

typedef QMap<Phonon::AudioDataOutput::Channel, QVector<float> > FloatMap;
typedef QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > IntMap;

inline void AudioDataOutput::convertAndEmit(const QVector<qint16> &leftBuffer, const QVector<qint16> &rightBuffer)
{
    //TODO: Floats
    IntMap map;
    map.insert(Phonon::AudioDataOutput::LeftChannel, leftBuffer);
    map.insert(Phonon::AudioDataOutput::RightChannel, rightBuffer);
    emit dataReady(map);
}

}} //namespace Phonon::VLC

#include "moc_audiodataoutput.cpp"
// vim: sw=4 ts=4

