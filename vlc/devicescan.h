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

/** \file
 * \brief Provides functions to detect available devices
 *
 * This file contains functions that scan for available devices on the system,
 * and will provide a list of DeviceInfo for the backend. Various classes of
 * devices can be supported. A number of libraries may be used to provide a list
 * for each class of devices.
 *
 * Currently supported: Video4Linux2 devices.
 * TODO ALSA devices, DirectShow devices.
 */

#ifndef Phonon_VLC_DEVICESCAN_H
#define Phonon_VLC_DEVICESCAN_H

#include "devicemanager.h"

#include <phonon/ObjectDescription>
#include <QtCore/QByteArray>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE

namespace Phonon {
namespace VLC {

#ifdef HAVE_LIBV4L2
namespace V4L2Support {

/**
 * Probes for V4L capture devices and appends them to the list.
 *
 * \param devices List of capture devices
 */
bool scanDevices(QList<DeviceInfo> & devices);

} // namespace V4L2Support
#endif // HAVE_LIBV4L2

#ifdef HAVE_LIBKAUDIODEVICELIST
#endif // HAVE_LIBKAUDIODEVICELIST

#ifdef HAVE_DIRECTSHOW
#endif // HAVE_DIRECTSHOW

} // namespace VLC
} // namespace Phonon

QT_END_NAMESPACE

#endif // Phonon_VLC_DEVICESCAN_H