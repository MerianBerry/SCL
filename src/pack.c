#include "scl/pack.h"
#include "scl/swiss.h"
#include "scl/fs.h"
#include "scl/time.h"
#include "uv.h"

#define SPK_VERSION     3

#define SPK_HEADER_SIZE 16
#define SPK_MAGIC       "SPK\x7f"
#define SPK_H_VERSION   4
#define SPK_H_FFLAGS    5


#define SPK_MAX_MEMBERS 32


typedef void (*scl_work_cb)(void* data, void* workptr);

/* essentially a job queue, given to thread workers */
typedef struct scl_workpage_t {
  scl_work_cb cb;
  /* user data */
  void* data;
  /* Only the array pointer is owned and freed by the page */
  void** ptrs;
  size_t count;

  struct scl_workpage_t* nextpage;
} scl_workpage_t;

typedef struct scl_worker_t {
  uv_mutex_t mut;
  uv_sem_t sem;
  uv_thread_t tid;

  /* external sources can only change headpage IF it is null.
   * or its nextpage pointer if it isnt.
   */
  scl_workpage_t* headpage;
  scl_workpage_t* tailpage;
  bool canceled;
} scl_worker_t;

typedef struct scl_workgroup_t {
  scl_worker_t* workers;
  uint8_t nworkers;
} scl_workgroup_t;

typedef struct scl_packfamily_t {
  scl_shmap_t* index;

} scl_packfamily_t;

typedef void (*scl_worker_cb)(struct scl_worker_t* info);

static int _reccomendedWorkgroup() {
  return uv_available_parallelism();
}

static int _initworkgroup(scl_workgroup_t* group, scl_worker_cb entry, int n) {
  memset(group, 0, sizeof(*group));
  n = max(0, min(n, 0xff));
  group->nworkers = n;
  group->workers = malloc(sizeof(scl_worker_t) * n);
  memset(group->workers, 0, sizeof(scl_worker_t) * n);

  for(int i = 0; i < n; i++) {
    scl_worker_t* iworker = &group->workers[i];
    if(uv_mutex_init(&iworker->mut))
      return -1;
    if(uv_thread_create(&iworker->tid, (uv_thread_cb)entry, (void*)iworker))
      return -1;
  }
  return 0;
}

static void _wgjoin(scl_workgroup_t* group) {
  if(!group || !group->workers || !group->nworkers)
    return;
  for(int i = 0; i < group->nworkers; i++) {
    uv_thread_join(&group->workers[i].tid);
  }
}

scl_packfamily_t* scl_packnew(const char* path) {
  if(!path)
    return NULL;
  if(!scl_fs_exists(path))
    return NULL;

  char header[SPK_HEADER_SIZE];
}

scl_pindex_t* scl_packopen(scl_packfamily_t* family, const char* path) {
}

scl_pindex_t** scl_packopens(
  scl_packfamily_t* family, uint32_t* count, const char* path) {
}
