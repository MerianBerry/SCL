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


/* sclc_swiss.c
 * swiss hash map implementation
 */
#include "xxhash.h"
#include <sclc_swiss.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef min
#  undef min
#  undef max
#endif
#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

#if INTPTR_MAX == INT64_MAX
#  define SWISS_MIN_CAP 8
/* ensures the size is a multiple of 8 */
#  define SWISS_SIZE_MASK ((uint32_t)~0b111)
#elif INTPTR_MAX == INT32_MAX
#  define SWISS_MIN_CAP   4
/* ensures the size is a multiple of 4 */
#  define SWISS_SIZE_MASK ((uint32_t)~0b11)
#endif

#define SWISS_MISSINGI 0xffffffff
#define SWISS_PMASK    0x80
#define SWISS_EMPTY    0x80
#define SWISS_DELETED  0xff

static uint64_t fasthash64_mix(uint64_t h) {
  h ^= h >> 23;
  h *= 0x2127599bf4325c37ULL;
  h ^= h >> 47;
  return h;
}

uint64_t fasthash64(const void* m_buf, size_t len, uint64_t seed) {
  const uint64_t m = 0x880355f21e6d1965ULL;
  const uint64_t* pos = (const uint64_t*)m_buf;
  const uint64_t* end = pos + (len / 8);
  const unsigned char* pos2;
  uint64_t h = seed ^ (len * m);
  uint64_t v;

  while(pos != end) {
    v = *pos++;
    h ^= fasthash64_mix(v);
    h *= m;
  }

  pos2 = (const unsigned char*)pos;
  v = 0;

  switch(len & 7) {
  case 7:
    v ^= (uint64_t)pos2[6] << 48;
  case 6:
    v ^= (uint64_t)pos2[5] << 40;
  case 5:
    v ^= (uint64_t)pos2[4] << 32;
  case 4:
    v ^= (uint64_t)pos2[3] << 24;
  case 3:
    v ^= (uint64_t)pos2[2] << 16;
  case 2:
    v ^= (uint64_t)pos2[1] << 8;
  case 1:
    v ^= (uint64_t)pos2[0];
    h ^= fasthash64_mix(v);
    h *= m;
  }

  return fasthash64_mix(h);
}

#define _hash64(ptr, size, seed) XXH3_64bits(ptr, size)

typedef struct shnode_t {
  const void* key;
  const void* data;
  uint64_t hash;
} shnode_t;

typedef struct scl_shmap_t {
  shnode_t* map;
  uint8_t* meta;
  uint32_t cap;
  uint32_t count;
} scl_shmap_t;

// Returns 1 if count is 75% or higher of cap. 75% chosen as max optimal
// capacity.
#define _shisover(map) ((map)->count >= (map)->cap >> 1)
// Resturns 1 if count is 12% or lower of cap, and cap isnt minimum
#define _shisunder(map) \
  (((map)->count <= (map)->cap >> 3) && ((map)->cap != SWISS_MIN_CAP))

scl_shmap_t* scl_shnew() {
  scl_shmap_t* ptr = (scl_shmap_t*)malloc(sizeof(scl_shmap_t));
  memset(ptr, 0, sizeof(*ptr));
  return ptr;
}

void scl_shfree(scl_shmap_t* map) {
  if(!map)
    return;
  if(map->map)
    free(map->map);
  if(map->meta)
    free(map->meta);
  free(map);
}

void scl_shclear(scl_shmap_t* map) {
  if(map->map)
    free(map->map);
  if(map->meta)
    free(map->meta);
  memset(map, 0, sizeof(*map));
}

#define _lowkey(hash)     ((hash) & 0x7f)
#define _basei(cap, hash) (((hash) % (cap)))

static shnode_t* _metamatch(
  const scl_shmap_t* map, uint32_t* i, uint32_t base, uint8_t lokey) {
  do {
    if(*i >= map->cap)
      *i = 0;
    if(map->meta[*i] == lokey)
      return &map->map[*i];
    (*i)++;
  } while(*i != base);
  return NULL;
}

static uint32_t _fullgeti(const scl_shmap_t* map, uint64_t hash) {
  shnode_t* node;
  const uint8_t lokey = _lowkey(hash);
  uint32_t base = _basei(map->cap, hash);
  uint32_t i = base;
  while((node = _metamatch(map, &i, base, lokey))) {
    if(node->hash == hash)
      return i;
    i++;
  }
  return SWISS_MISSINGI;
}

static shnode_t* _fullget(const scl_shmap_t* map, uint64_t hash) {
  uint32_t i = _fullgeti(map, hash);
  if(i == SWISS_MISSINGI)
    return NULL;
  return &map->map[i];
}

const void* scl_shget(const scl_shmap_t* map, uint64_t hash) {
  if(!map || !map->map)
    return NULL;
  shnode_t* node = _fullget(map, hash);
  if(!node)
    return NULL;
  return node->data;
}

const void* scl_shgets(const scl_shmap_t* map, const char* key) {
  if(!key)
    return NULL;
  uint64_t hash = _hash64(key, strlen(key), 1024);
  return scl_shget(map, hash);
}

bool scl_shhas(const scl_shmap_t* map, uint64_t hash) {
  if(!map || !map->map)
    return false;
  shnode_t* node = _fullget(map, hash);
  return !!node;
}

bool scl_shhass(const scl_shmap_t* map, const char* key) {
  if(!key)
    return false;
  uint64_t hash = _hash64(key, strlen(key), 1024);
  return scl_shhas(map, hash);
}

const void* scl_shkey(const scl_shmap_t* map, uint64_t hash) {
  if(!map || !map->map)
    return NULL;
  shnode_t* node = _fullget(map, hash);
  if(!node)
    return NULL;
  return node->key;
}

static bool _put(scl_shmap_t* map, shnode_t node, uint32_t cap) {
  shnode_t* pnode;
  const uint8_t lokey = _lowkey(node.hash);
  uint32_t base = _basei(cap, node.hash);
  const uint32_t end = min(base - 1, cap);
  uint32_t i = base;
  /* find the next empty node. */
  while(1) {
    /* true collision */
    if(map->meta[i] == lokey && map->map[i].hash == node.hash) {
      /* update data */
      map->map[i] = node;
      return true;
    }
    if((map->meta[i] & SWISS_PMASK))
      break;
    i++;
    if(i == end)
      return false; /* round-trip search (no empty nodes), end with a failure */
    if(i >= cap)
      i = 0;
  }

  map->meta[i] = lokey;
  map->map[i] = node;
  map->count++;
  return true;
}

static void _rehash(scl_shmap_t* map, uint32_t newcap) {
  if(!map->cap)
    return;
  const uint32_t oldcount = map->count;
  uint32_t i;
  for(i = 0; i < map->cap; i++) {
    /* if the node is not full, skip */
    if(map->meta[i] & SWISS_PMASK)
      continue;
    map->meta[i] = SWISS_EMPTY;
    _put(map, map->map[i], newcap);
  }
  map->count = oldcount;
}

static void _ensuresize(scl_shmap_t* map) {
  if(!_shisover(map) && !_shisunder(map))
    return;
  const uint32_t newcap =
    ((map->count << 2) & SWISS_SIZE_MASK) + SWISS_MIN_CAP; /* aim for 50% */

  if(newcap < map->cap)
    _rehash(map, newcap);

  map->map = realloc(map->map, sizeof(shnode_t) * newcap);
  map->meta = realloc(map->meta, newcap);

  if(newcap > map->cap) {
    memset(map->meta + map->cap, SWISS_EMPTY, newcap - map->cap);
    _rehash(map, newcap);
  }
  map->cap = newcap;
}

bool scl_shset(
  scl_shmap_t* map, const void* key, uint64_t hash, const void* data) {
  if(!map)
    return false;
  _ensuresize(map);
  shnode_t node = {.key = key, .data = data, .hash = hash};
  return _put(map, node, map->cap);
}

bool scl_shsets(scl_shmap_t* map, const char* key, const void* data) {
  if(!key)
    return false;
  uint64_t hash = _hash64(key, strlen(key), 1024);
  return scl_shset(map, key, hash, data);
}

void scl_shremove(scl_shmap_t* map, uint64_t hash) {
  if(!map || !map->map)
    return;
  uint32_t i = _fullgeti(map, hash);
  if(i == SWISS_MISSINGI)
    return;
  map->meta[i] = SWISS_DELETED;
  map->count--;
}

void scl_shremoves(scl_shmap_t* map, const char* key) {
  if(!key)
    return;
  uint64_t hash = _hash64(key, strlen(key), 1024);
  scl_shremove(map, hash);
}

uint32_t scl_shlength(const scl_shmap_t* map) {
  if(!map)
    return 0;
  return map->count;
}

uint32_t scl_shcapacity(const scl_shmap_t* map) {
  if(!map)
    return 0;
  return map->cap;
}
