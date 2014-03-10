/*
    Copyright (C) 2014 Harald Sitter <sitter@kde.org>

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

#ifndef PHONON_DEBUG_H
#define PHONON_DEBUG_H

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(PHONON_BACKEND_VLC)

#define DEBUG_BLOCK
// FIXME: these tend to clash with gcc weeeh - cannot be defines because of that
inline QDebug debug()   { return qCDebug(PHONON_BACKEND_VLC); }
inline QDebug warning() { return qCWarning(PHONON_BACKEND_VLC); }
inline QDebug error()   { return warning(); }
inline QDebug fatal()   { return qCCritical(PHONON_BACKEND_VLC); }

#endif // PHONON_DEBUG_H
