/*****************************************************************************
* libVLC backend for the Phonon library                                     *
*                                                                           *
* Copyright (C) 2010 Casian Andrei <skeletk13@gmail.com>                    *
* Copyright (C) 2010 vlc-phonon AUTHORS                                     *
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

#ifndef Phonon_VLC_V4L2DEVICES_H
#define Phonon_VLC_V4L2DEVICES_H

#ifdef HAVE_LIBV4L2

#include "devicemanager.h"

#include <phonon/ObjectDescription>
#include <QtCore/QByteArray>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE

namespace Phonon {
namespace VLC {
namespace V4L2Support {

bool scanDevices(QList<DeviceInfo> & devices);

}
}
} // namespace Phonon::VLC::V4L2Support

QT_END_NAMESPACE

#endif // HAVE_LIBV4L2

#endif // Phonon_VLC_V4L2DEVICES_H