/*
  Copyright (c) 2026 Merian

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/* sclc_stream.h
 * adds io helpers
 */

#ifndef sclc_stream_h
#define sclc_stream_h

#ifdef __cplusplus
extern "C" {
#endif

#include "sclc_base.h"
#include <stdio.h>

typedef struct scl_stream_t scl_stream_t;

typedef enum {
  SCL_SEEK_START = SEEK_SET,
  SCL_SEEK_CURRENT = SEEK_CUR,
  SCL_SEEK_END = SEEK_END,
} scl_seekpos;

extern SCLAPI scl_stream_t* scl_streamnew();

extern SCLAPI bool scl_fisopen(const scl_stream_t* stream);

extern SCLAPI bool scl_fismodified(const scl_stream_t* stream);

extern SCLAPI size_t scl_ftell(const scl_stream_t* stream);

extern SCLAPI size_t scl_fsize(const scl_stream_t* stream);

extern SCLAPI void scl_funmodify(scl_stream_t* stream);

/**
 * @brief Opens the stream object with the given path and mode.
 *
 * @param  path  Filepath to attempt to open.
 * @param  mode  Normal C fopen mode string. If null (path must also be null),
 * opens in memory mode.
 * @return Returns a non-null pointer if successful, returns null if failed.
 */
extern SCLAPI scl_stream_t* scl_fopen(const char* path, const char* mode);

extern SCLAPI void scl_fclose(scl_stream_t* stream);

/**
 * @brief Flushes internal buffers. Does nothing in memory mode.
 */
extern SCLAPI void scl_fflush(scl_stream_t* stream);

extern SCLAPI size_t scl_fseek(scl_seekpos pos, ssize_t off);

extern SCLAPI ssize_t scl_fread(scl_stream_t* stream, void* buf, size_t n);

extern SCLAPI bool scl_freserve(scl_stream_t* stream, size_t n, bool force);

extern SCLAPI bool scl_fwrite(scl_stream_t* stream, const void* buf, size_t n);

extern SCLAPI bool scl_fprint(scl_stream_t* stream, const char* str);

extern SCLAPI bool scl_fwrites(scl_stream_t* stream, scl_stream_t* stream2);

/**
 * @brief Gives the pointer to the internal data buffer, if in data mode.
 * @warning Do not free this pointer. This pointer can be invalidated by doing
 * work on the stream, or closing it.
 * @param  stream
 * @return Returns a pointer to the internal data buffer, or null if not
 * available.
 */
extern SCLAPI const void* scl_fdata(scl_stream_t* stream);

/**
 * @brief  Releases the internal data buffer from the streams control, if in
 * memory mode. The stream will be reset after this call.
 *
 * @return  Pointer to this streams internal data buffer. If valid, you must
 * free it.
 */
extern SCLAPI void* scl_frealease(scl_stream_t* stream);

#ifdef __cplusplus
}
#endif
#endif
