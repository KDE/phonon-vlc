/*****************************************************************************
 * libVLC backend for the Phonon library                                     *
 *                                                                           *
 * Stream support                                                            *
 *                                                                           *
 * Copyright (C) 2010 Daniel O'Neill <ver@gangrenegang.com>                  *
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
