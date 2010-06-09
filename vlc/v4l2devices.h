/*  This file is part of the KDE project.

Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).

This library is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 or 3 of the License.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef Phonon_VLC_V4L2DEVICES_H
#define Phonon_VLC_V4L2DEVICES_H

//#ifdef Q_OS_LINUX
//#define HAVE_LIBV4L2
//#ifdef HAVE_LIBV4L2

#include <phonon/ObjectDescription>
#include <QtCore/QByteArray>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE

namespace Phonon {
namespace VLC {
namespace V4L2Support {

static bool scanDevices(QList<QByteArray> & videoCaptureDeviceNames, QList<QByteArray> & audioCaptureDeviceNames);

}
}
} // namespace Phonon::VLC::V4L2Support

QT_END_NAMESPACE

//#endif // HAVE_LIBV4L2
//#endif // Q_OS_LINUX

#endif // Phonon_VLC_V4L2DEVICES_H