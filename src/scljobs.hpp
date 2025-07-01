/*  scljobs.cpp
 *  SCL work multithreading library
 */
#ifndef JOBS_H
#define JOBS_H

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <atomic>
#include "sclcore.hpp"

#ifndef SCL_JOBS_FAST_SLEEP
#  define SCL_JOBS_FAST_SLEEP 0.001
#endif
#ifndef SCL_JOBS_SLOW_SLEEP
#  define SCL_JOBS_SLOW_SLEEP 1
#endif
#define SCL_JOBS_SLEEP(sl) (!(sl) ? SCL_JOBS_FAST_SLEEP : SCL_JOBS_SLOW_SLEEP)

namespace scl {
namespace jobs {

template <class WtT>
class job;
class JobWorker;

class waitable {
  template <class Wt>
  friend class job;
  using _Waitable = bool;

 private:
  std::atomic_bool m_done;

 protected:
 public:
  waitable();

  void complete();
  bool wait(double timeout = -1);
};

class JobWorker;
class JobServer;

template <class WtT>
class job {
  friend class JobServer;
  friend class JobWorker;

 private:
  bool                                 autodelwt    = false;
  static const typename WtT::_Waitable _is_waitable = 1;

 protected:
 public:
  using Wt                         = WtT;
  virtual ~job()                   = default;

  virtual Wt  *getWaitable() const = 0;

  virtual bool checkJob(const JobWorker &worker) const {
    return true;
  }

  virtual void doJob(Wt *waitable, const JobWorker &worker) = 0;
};

class funcJob : public job<waitable> {
  std::function<void(const JobWorker &worker)> m_func;

 public:
  funcJob(std::function<void(const JobWorker &worker)> func);

  waitable *getWaitable() const override;

  void      doJob(waitable *waitable, const JobWorker &worker) override;
};

class JobWorker {
  friend class JobServer;
  scl::string      m_desc;
  JobServer       *m_serv;
  std::atomic_bool m_working;
  std::atomic_bool m_busy;
  int              m_id;

  void             quit();

 public:
  JobWorker(JobServer *serv, int id);

  JobServer  &serv() const;
  void        sync(const std::function<void()> &func) const;

  int         id() const;
  bool        working() const;
  bool        busy() const;

  static void work(JobWorker *inst);
};

class JobServer : protected std::mutex {
  using t_worker = std::pair<std::thread, JobWorker *>;
  using t_wjob   = std::pair<job<waitable> *, waitable *>;
  friend class JobWorker;
  std::vector<t_worker> m_workers;
  std::queue<t_wjob>    m_jobs;
  std::atomic<size_t>   m_lockBits;
  int                   m_nworkers;
  std::atomic_bool      m_slow;
  std::atomic_bool      m_working;


  bool                  takeJob(t_wjob &wjob, const JobWorker &worker);

 public:
  JobServer(int workers = 0);
  ~JobServer();

  JobServer(const JobServer &)      = delete;
  JobServer &operator=(JobServer &) = delete;

  bool       is_working() const;

  void       start();
  void       slow(bool state = true);
  bool       waitidle(double timeout = -1);
  void       stop();

  void       setLockBits(size_t bits);
  void       unsetLockBits(size_t bits);
  bool       hasLockBits(size_t bits) const;

  void       clearjobs();

  void       sync(const std::function<void()> &func);

  template <class Jb>
  typename Jb::Wt &submitJob(Jb *job, bool autodelwt = false) {
    waitable *wt = job->getWaitable();
    lock();
    scl::jobs::job<waitable> *job_ = (scl::jobs::job<waitable> *)job;
    job_->autodelwt                = autodelwt;
    m_jobs.push(t_wjob(job_, wt));
    unlock();
    return (typename Jb::Wt &)*wt;
  }

  waitable   *submitJob(std::function<void(const JobWorker &worker)> func,
      bool autodelwt = true);

  int         workerCount() const;

  static int  getnthreads(int threads);

  static void multithread(std::function<void(int id, int workers)> func,
    int                                                            workers = 0);
};
} // namespace jobs
} // namespace scl
#endif
