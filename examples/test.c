#include "scl/fs.h"
#include "scl/glob.h"
#include "uv.h"
#include "scl/path.h"
#include "scl/pack.h"
#include "scl/time.h"

static void scan_sync(uv_loop_t* loop, const char* path) {
  uv_fs_t req;
  uv_fs_scandir(loop, &req, path, 0, NULL);

  if(req.result < 0)
    return;

  uv_dirent_t ent;
  while(uv_fs_scandir_next(&req, &ent) != UV_EOF) {
    char* path = NULL;
    scl_pathjoin(path, req.path, ent.name);
    fprintf(stderr, "%s\n", path);
    if(ent.type == UV_DIRENT_DIR) {
      scan_sync(loop, path);
    }
  }

  uv_fs_req_cleanup(&req);
}

static void _gelp1(uv_work_t* req) {
  fprintf(stderr, "sleeping...\n");
  scl_waitms(1000);
  fprintf(stderr, "slept :)\n");
  uv_sem_post(req->data);
  req->data = (void*)1;
}

static void _gelp2(uv_work_t* req) {
  uv_sem_wait(req->data);
  fprintf(stderr, "my body is ready\n");
  req->data = (void*)2;
}

static void _gelp0(uv_work_t* req, int status) {
  fprintf(stderr, "done :) %i\n", (int)(intptr_t)req->data);
}

int main(int argc, char** argv) {
  uv_setup_args(argc, argv);

  uv_os_setenv("UV_THREADPOOL_SIZE", "8");

  uv_loop_t* loop = uv_default_loop();

#if 0
  scan_sync(loop, "test");

  uv_fs_t req;
  int r = uv_fs_open(loop, &req, "CMakeLists.txt", 0, O_RDONLY, NULL);
  if(req.result < 0)
    return 1;
  uv_file fd = req.result;

  char buf[1024];
  uv_buf_t iov = uv_buf_init(buf, sizeof(buf));
  int read = 1;
  while(read) {
    read = uv_fs_read(loop, &req, fd, &iov, 1, -1, NULL);
    fwrite(buf, 1, read, stderr);
    iov = uv_buf_init(buf, sizeof(buf));
    if(read < sizeof(buf))
      break;
  }
  fprintf(stderr, "\n");
#endif


#if 0
  uv_sem_t sem;
  uv_sem_init(&sem, 0);
  uv_work_t work;
  work.data = &sem;
  uv_queue_work(loop, &work, _gelp1, _gelp0);

  uv_work_t work2;
  work2.data = &sem;
  uv_queue_work(loop, &work2, _gelp2, _gelp0);

  uv_run(loop, UV_RUN_NOWAIT);

  uv_run(loop, UV_RUN_DEFAULT);

#endif

  uv_loop_close(loop);
}
