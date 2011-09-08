/*
    Copyright (c) 2011 Harald Sitter <sitter@kde.org>

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

#ifndef PHONON_VLC_DEBUG_H
#define PHONON_VLC_DEBUG_H

#define DAVROS_DEBUG_AREA "PHONON-VLC"

#include <davros/davros.h>
#include <davros/block.h>

#define DEBUG_BLOCK DAVROS_BLOCK

#define debug Davros::debug
#define warning Davros::warning
#define error Davros::error
#define fatal Davros::fatal

#endif // PHONON_VLC_DEBUG_H
