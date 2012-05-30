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

#ifndef PHONON_VLC_VSTRING_H
#define PHONON_VLC_VSTRING_H

#include <QtCore/QString>
#include <vlc/libvlc.h>
#include <vlc/libvlc_version.h>

class VString
{
public:
    VString(char *vlcString) : m_vlcString(vlcString) {}
    ~VString()
    {
#if (LIBVLC_VERSION_INT >= LIBVLC_VERSION(2, 0, 0, 0))
        libvlc_free(m_vlcString);
#else
        free(m_vlcString);
#endif // >= VLC 2
    }

    // VLC internally only uses UTF8!
    QString toQString() { return QString::fromUtf8(m_vlcString); }
    operator QString() { return toQString(); }

private:
    VString() {}

    char *m_vlcString;
};

#endif // PHONON_VLC_VSTRING_H
