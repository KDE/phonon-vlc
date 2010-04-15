#include "streamreader.h"
#include "streamhooks.h"

#define BLOCKSIZE 32768

using namespace Phonon::VLC;

int c_stream_read( void *p_streamReaderCtx, size_t *i_length, char **p_buffer )
{
    StreamReader *streamReader = (StreamReader *)p_streamReaderCtx;
    size_t length = BLOCKSIZE;

    char *buffer = new char[length];
    *p_buffer = buffer;

    int srSize = length;
    bool b_ret = streamReader->read( streamReader->currentPos(), &srSize, buffer );
    length = srSize;

    *i_length = length;

    return b_ret ? 0 : -1;
}

int c_stream_seek( void *p_streamReaderCtx, const uint64_t i_pos )
{
    StreamReader *streamReader = (StreamReader *)p_streamReaderCtx;
    if( i_pos > streamReader->streamSize() )
    {
        // attempt to seek past the end of our data.
        return -1;
    }

    streamReader->setCurrentPos( i_pos );
    // this should return a true/false, but it doesn't, so assume success.

    return 0;
}

extern "C"
{
    int streamReadCallback(void *data, const char *cookie,
                           int64_t *dts, int64_t *pts, unsigned *flags,
                           size_t *sz, void **buf)
    {
        return c_stream_read( data, sz, (char **)buf );
    }

    int streamSeekCallback(void *data, const uint64_t pos)
    {
        return c_stream_seek( data, pos );
    }

    int streamReadDoneCallback(void *data, const char *cookie, size_t sz, void *buf)
    {
        delete buf;
    }
}

