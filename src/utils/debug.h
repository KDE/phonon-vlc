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

// FIXME: do something with debug_block or drop it
#define DEBUG_BLOCK

#define pDebug() qCDebug(PHONON_BACKEND_VLC)
#define pWarning() qCWarning(PHONON_BACKEND_VLC)
#define pCritical() qCCritical(PHONON_BACKEND_VLC)
#define pFatal qFatal

#endif // PHONON_DEBUG_H
