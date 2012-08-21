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

#ifndef PHONON_VLC_MIME_H
#define PHONON_VLC_MIME_H

#include <QtCore/QStringList>

namespace Phonon {
namespace VLC {

static QStringList mimeTypeList()
{
    # In CMake we null-terminate this list for ease of iteration.
    const char *c_strings[] = @PHONON_VLC_MIME_TYPES_C_ARRAY@;

    QStringList list;
    int i = 0;
    while (c_strings[i])
        list.append(QLatin1String(c_strings[i]));
    return list;
}

} // namespace VLC
} // namespace Phonon

#endif // PHONON_VLC_MIME_H
