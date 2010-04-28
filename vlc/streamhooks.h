/******************************************************************************
 * streamhook.h: Stream support
 ****************************************************************************
 *  This file is part of the Phonon/VLC project
 *
 * Copyright (C) Daniel O'Neill <ver -- gangrenegang dot com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), and the KDE Free Qt
 * Foundation, which shall act as a proxy defined in Section 6 of version 3 of
 * the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/


#ifndef STREAMHOOKS_H
#define STREAMHOOKS_H

#include <stdint.h>

int c_stream_read( void *p_streamReaderCtx, size_t *i_length, char **p_buffer );
int c_stream_seek( void *p_streamReaderCtx, const uint64_t i_pos );
extern "C"
{
    int streamReadCallback(void *data, const char *cookie,
                           int64_t *dts, int64_t *pts, unsigned *flags,
                           size_t *, void **);
    int streamReadDoneCallback(void *data, const char *cookie, size_t, void *);
    int streamSeekCallback(void *data, const uint64_t pos);
}

#endif // STREAMHOOKS_H
