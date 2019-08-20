/*
    Copyright (C) 2019 Harald Sitter <sitter@kde.org>

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

#ifndef PHONON_VLC_CONFIG_H
#define PHONON_VLC_CONFIG_H

// I'd really love to have inline constexpr QStringViews here, I'm unsure what
// that requires vis-a-vis compiler support though :(

#cmakedefine PHONON_VLC_VERSION "@PHONON_VLC_VERSION@"
#cmakedefine PHONON_EXPERIMENTAL

#endif // PHONON_VLC_CONFIG_H
