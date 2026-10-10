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


/* pack.h
 *
 */

#ifndef SCL_PACK_H
#define SCL_PACK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "base.h"

#define SCL_PINDEX_ACTIVE    1
#define SCL_PINDEX_SUBMITTED 2

typedef struct scl_packfamily_t scl_packfamily_t;

typedef struct scl_pindex_t {
  const char* name;
  /* file content */
  const void* content;
  /* pack offset */
  uint32_t offset;
  /* compressed size */
  uint32_t csize;
  /* original size */
  uint32_t osize;
  uint8_t pack;
  uint8_t flags;
  bool complete;
} scl_pindex_t;

SCLLIB scl_packfamily_t* scl_packnew(const char* path);

SCLLIB scl_pindex_t* scl_packopen(scl_packfamily_t* family, const char* path);

SCLLIB scl_pindex_t** scl_packopens(
  scl_packfamily_t* family, uint32_t* count, const char* path);


#ifdef __cplusplus
}
#endif
#endif
