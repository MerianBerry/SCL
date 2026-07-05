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

typedef struct scl_stream scl_stream;

typedef enum {
  SCL_SEEK_START = SEEK_SET,
  SCL_SEEK_CURRENT = SEEK_CUR,
  SCL_SEEK_END = SEEK_END,
} scl_seekpos;

scl_stream* scl_streamnew();

bool scl_fisopen(const scl_stream* stream);

bool scl_fismodified(const scl_stream* stream);

size_t scl_ftell(const scl_stream* stream);

size_t scl_fsize(const scl_stream* stream);

void scl_funmodify(scl_stream* stream);

/**
 * @brief Opens the stream object with the given path and mode.
 *
 * @param  path  Filepath to attempt to open.
 * @param  mode  Normal C fopen mode string. If null (path must also be null),
 * opens in memory mode.
 * @return Returns a non-null pointer if successful, returns null if failed.
 */
scl_stream* scl_fopen(const char* path, const char* mode);

void scl_fclose(scl_stream* stream);

/**
 * @brief Flushes internal buffers. Does nothing in memory mode.
 */
void scl_fflush(scl_stream* stream);

size_t scl_fseek(scl_seekpos pos, ssize_t off);

ssize_t scl_fread(scl_stream* stream, void* buf, size_t n);

bool scl_freserve(scl_stream* stream, size_t n, bool force);

bool scl_fwrite(scl_stream* stream, const void* buf, size_t n);

bool scl_fprint(scl_stream* stream, const char* str);

bool scl_fwrites(scl_stream* stream, scl_stream* stream2);

/**
 * @brief Gives the pointer to the internal data buffer, if in data mode.
 * @warning Do not free this pointer. This pointer can be invalidated by doing
 * work on the stream, or closing it.
 * @param  stream
 * @return Returns a pointer to the internal data buffer, or null if not
 * available.
 */
const void* scl_fdata(scl_stream* stream);

/**
 * @brief  Releases the internal data buffer from the streams control, if in
 * memory mode. The stream will be reset after this call.
 *
 * @return  Pointer to this streams internal data buffer. If valid, you must
 * free it.
 */
void* scl_frealease(scl_stream* stream);

#ifdef __cplusplus
}
#endif
#endif
