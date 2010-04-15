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
