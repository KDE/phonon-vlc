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

/**
 * @brief The VString class wraps around a char* returned from libvlc functions.
 * Directly from libvlc functions returned cstrings are unique in two ways.
 * For one they are to be freed by the caller, and for another they are always
 * UTF8 (as VLC internally only uses UTF8).
 *
 * Particularly the first point is where VString comes in, it will on destruction
 * call libvlc_free to free the string, thus avoiding memleaks.
 * Additionally it conveniently converts to QString using either toQString
 * or implicit cast to QString which makes it completely transparent to other
 * functions. It also prevents you from carrying the cstring out of scope and
 * render implicit copies of it invalid once free is called somewhere.
 * Both functions use QString::fromUtf8 and are therefore the best way to
 * process the string.
 */
class VString
{
public:
    VString(char *vlcString) : m_vlcString(vlcString) {}
    ~VString()
    {
        libvlc_free(m_vlcString);
    }

    // VLC internally only uses UTF8!
    QString toQString() { return QString::fromUtf8(m_vlcString); }
    operator QString() { return toQString(); }

private:
    VString() {}

    char *m_vlcString;
};

#endif // PHONON_VLC_VSTRING_H
