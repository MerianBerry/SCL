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


/* sclc_swiss.h
 * swiss table declarations
 */

#ifndef sclc_swiss_h
#define sclc_swiss_h

#ifdef __cplusplus
extern "C" {
#endif

#include "sclc_base.h"

typedef struct scl_shmap_t scl_shmap_t;

SCLAPI scl_shmap_t* scl_shnew();

SCLAPI void scl_shfree(scl_shmap_t* map);

SCLAPI const void* scl_shget(const scl_shmap_t* map, uint64_t hash);

SCLAPI const void* scl_shgets(const scl_shmap_t* map, const char* key);

SCLAPI const void* scl_shkey(const scl_shmap_t* map, uint64_t hash);

SCLAPI void scl_shset(
  const scl_shmap_t* map, const void* key, uint64_t hash, const void* data);

SCLAPI void scl_shsets(
  const scl_shmap_t* map, const char* key, const void* data);

SCLAPI void scl_shremove(const scl_shmap_t* map, uint64_t hash);

SCLAPI void scl_shremoves(const scl_shmap_t* map, const char* key);

#ifdef __cplusplus
}
#endif
#endif
