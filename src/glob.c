
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

#include "scl/glob.h"
#include "scl/fs.h"
#include "scl/string.h"
#include "scl/path.h"
#include <stdlib.h>
#include <string.h>

static const char** glob_unit(
  const char* dir, const char* mask, const char** finds, int mode) {
  char buf[PATH_MAX];

  /* if the mask isnt wild, just check if it is in the right mode */
  if(!scl_iswild(mask)) {
    /* dont join . to paths */
    if(!!strcmp(dir, "."))
      scl_pathjoinx(buf, dir, mask);
    else
      memcpy(buf, mask, strlen(mask) + 1);
    if(mode == SCL_GLOB_FILES && !scl_fs_isfile(buf))
      return finds;
    else if(mode == SCL_GLOB_DIRS && !scl_fs_isdirectory(buf))
      return finds;
    scl_svappend(finds, buf, 1024);
    return finds;
  }

  uv_fs_t oreq;
  if(uv_fs_scandir(uv_default_loop(), &oreq, dir, 0, NULL) < 0)
    return NULL;

  /* capture the dir pointer */
  const char* fixdir = strcopy(dir);

  uv_dirent_t ent;
  while(uv_fs_scandir_next(&oreq, &ent) != UV_EOF) {
    const char* item = ent.name;

    /* dont join . to paths */
    if(!!strcmp(fixdir, "."))
      scl_pathjoinx(buf, fixdir, item);
    else
      memcpy(buf, item, strlen(item) + 1);

    if(mode == SCL_GLOB_FILES && !scl_fs_isfile(buf))
      continue;
    else if(mode == SCL_GLOB_DIRS && !scl_fs_isdirectory(buf))
      continue;

    if(strmatch(item, mask)) {
      scl_svappend(finds, buf, 1024);
    }
  }
  free((void*)fixdir);
  uv_fs_req_cleanup(&oreq);
  return finds;
}

static const char** glob_expandall(const char* mask, const char** dirs) {
  /* Expand each in dirs, output into dirs. */
  const char** searches = dirs;
  dirs = NULL;
  for(size_t i = 0; searches[i]; i++) {
    const char* dir = searches[i];
    searches = glob_unit(searches[i], "*", searches, SCL_GLOB_DIRS);
    const char* tmp = scl_filename(searches[i]);
    if(strmatch(tmp, mask)) {
      scl_svappend(dirs, searches[i], 1024);
    }
    if(tmp)
      free((void*)tmp);
  }
  scl_svfree(searches);
  return dirs;
}

static const char** glob_split(const char* pattern) {
  /* Split all contiguous paths, wild components, and final component.
   * Should only have 1 result if the pattern is a component without a path.
   */

  const char** splits = NULL;
  const char *s = pattern, *p = s;
  while(*s) {
    const char* slash = strstr(p, "/");
    const char* wild = strstr(p, "*");
    const char* beforep = max(p - 1, s);

    if((!slash || (wild && wild < slash))) {
      /* add the stored split */
      if(s != p) {
        const char* split = strsub(s, 0, beforep - s);
        scl_svappend(splits, split, 32);
        free((void*)split);
      }

      const char* split;
      if(slash)
        split = strsub(p, 0, slash - p);
      else
        split = p;
      scl_svappend(splits, split, 32);
      if(slash) {
        free((void*)split);
        s = slash + 1;
      } else
        break;
    }

    p = slash + 1;
  }
  return splits;
}

static const char** glob_pattern(
  const char** finds, const char* pattern, int mode) {
  /* get all the expandable "splits" of the pattern */
  const char** splits = glob_split(pattern);
  if(!splits)
    return NULL;

#if 0
  for(int i = 0; i < scl_svlen(splits); i++)
    fprintf(stderr, "%s\n", splits[i]);
#endif

  const char** dirs = NULL;
  const char** globs = splits;
  /* if necessary for a succesfull search, add . to the dir searches */
  if(scl_svlen(splits) == 1 && !strstr(splits[0], "/"))
    scl_svappend(dirs, ".", 128);
  if(scl_svlen(splits) > 1) {
    /* wilds must be expanded, so they cant be added to dirs */
    if(scl_iswild(splits[0]))
      scl_svappend(dirs, ".", 128);
    else {
      scl_svappend(dirs, splits[0], 128);
      /* the first split is already accounted for */
      globs = splits + 1;
    }
  }

  /* For every dir glob */
  for(; *globs; globs++) {
    size_t i = globs - splits;
    if(!strncmp(*globs, "**", 3)) {
      const char* mask = "*";
      /* If not in the last 2 splits, use the next exp as the mask */
      if(i + 2 < scl_svlen(splits))
        mask = *(globs + 1);
      dirs = glob_expandall(mask, dirs);
      /* if not the last exp */
    } else if(*(globs + 1)) {
      /* using the existing dirs, find new ones based off of the mask */
      const size_t k = scl_svlen(dirs);
      for(size_t j = 0; j < k; j++)
        dirs = glob_unit(dirs[j], *globs, dirs, SCL_GLOB_DIRS);
    }
  }

  const char* fn = scl_svback(splits);
  // Find requested items
  for(size_t i = 0; i < scl_svlen(dirs); i++)
    finds = glob_unit(dirs[i], fn, finds, mode);
  scl_svfree(splits);
  scl_svfree(dirs);
  return finds;
}

static const char* strcomponent(const char** path, const char* split) {
  if(!path || !*path)
    return NULL;
  const char *s = *path, *p = s, *e = s;
  e = strstr(s, split);
  p = e + 1;
  /* split couldnt be found, so terminate path and copy remaining string */
  if(!e) {
    /* send path to null terminator */
    *path += strlen(*path);
    return strcopy(s);
  }
  if(e == s)
    e = p;
  *path = p;
  if(e == s)
    return NULL;
  const char* component = strsub(s, 0, e - s);
  if(component && strstr(component, "**")) {
    free((void*)component);
    component = strcopy("**");
  }
  return component;
}

static const char** glob_subroot(
  const char** finds, const char* search, int mode) {
  /* TODO: parse pattern for conditional wilds (x|y|z).
   * conditional wilds decomposes into n searches with the wild replaced with
   * the respective cond.
   */
  const char* cs = strstr(search, "(");
  const char* ce = cs ? strstr(cs, ")") : NULL;
  /* no conditions, normal search */
  if(!cs || !ce)
    return glob_pattern(finds, search, mode);

  const int32_t i = cs - search;
  const int32_t j = ce - cs + 1;
  /* find all conditionals, split by | */
  const char* conditions = strsub(cs, 1, j - 2);
  const char* p = conditions;
  while(*p) {
    const char* component = strcomponent(&p, "|");
    if(!component)
      break;
    const char* decomp = strreplace(search, component, i, j);
    free((void*)component);

    finds = glob_subroot(finds, decomp, mode);
    free((void*)decomp);
  }
  return finds;
}

static int glob_root(scl_glob_t* glob, int mode) {
  const char** finds = glob->finds;
  const char *s = glob->pattern, *p = s;

  /* split separate searches */
  while(p) {
    p = strstr(s, ";");
    const char* search = s;
    if(p)
      search = strsub(s, 0, p - s);

    finds = glob_subroot(finds, search, mode);

    if(p)
      free((void*)search);
    s = p + 1;
  }
  glob->finds = finds;
  glob->count = scl_svlen(finds);
  return 0;
}

void scl_globfree(scl_glob_t* glob) {
  if(!glob)
    return;
  scl_svfree(glob->finds);
  if(glob->pattern)
    free((void*)glob->pattern);
}

int scl_glob(scl_glob_t* glob, int mode) {
  if(!glob || (glob->count ^ !!glob->finds) || !glob->pattern)
    return -1;
  return glob_root(glob, mode);
}
