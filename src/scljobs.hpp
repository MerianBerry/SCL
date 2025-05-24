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

#define SCL_JOBS_FAST_SLEEP 0.001
#define SCL_JOBS_SLOW_SLEEP 1
#define SCL_JOBS_SLEEP(sl)  (!(sl) ? SCL_JOBS_FAST_SLEEP : SCL_JOBS_SLOW_SLEEP)

namespace scl {
namespace jobs {

template <class WtT>
class job;

class waitable : protected std::mutex {
  template <class Wt>
  friend class job;
  using _Waitable = bool;

 private:
  bool m_done = false;

 protected:
 public:
  void complete();

  bool wait (double timeout = -1);
};

class jobworker;
class jobserver;

template <class WtT>
class job {
  friend class jobserver;
  friend class jobworker;

 private:
  bool                                 autodelwt    = false;
  static typename WtT::_Waitable const _is_waitable = 1;

 protected:
 public:
  using Wt       = WtT;
  virtual ~job() = default;

  virtual Wt *getWaitable() const = 0;

  virtual void doJob (Wt *waitable, jobworker const &worker) = 0;
};

class funcJob : public job<waitable> {
  std::function<void (jobworker const &worker)> m_func;

 public:
  funcJob (std::function<void (jobworker const &worker)> func);

  waitable *getWaitable() const override;

  void doJob (waitable *waitable, jobworker const &worker) override;
};

class jobworker {
  friend class jobserver;
  scl::string      m_desc;
  jobserver       *m_serv;
  std::atomic_bool m_working;
  std::atomic_bool m_busy;
  int              m_id;

  void quit();

 public:
  jobworker (jobserver *serv, int id);

  void sync (std::function<void()> const &func) const;

  int  id() const;
  bool working() const;
  bool busy() const;

  static void work (jobworker *inst);
};

class jobserver : protected std::mutex {
  using t_worker = std::pair<std::thread, jobworker *>;
  using t_wjob   = std::pair<job<waitable> *, waitable *>;
  friend class jobworker;
  std::vector<t_worker> m_workers;
  std::queue<t_wjob>    m_jobs;
  int                   m_nworkers;
  std::atomic_bool      m_slow;
  std::atomic_bool      m_working;

  static int getNWorkers (int workers);

  bool takeJob (t_wjob &wjob);

 public:
  jobserver (int workers = 0);
  ~jobserver();

  jobserver (jobserver const &)      = delete;
  jobserver &operator= (jobserver &) = delete;

  void start();
  void slow (bool state = true);
  bool waitidle (double timeout = -1);
  void stop();

  void clearjobs();

  void sync (std::function<void()> const &func);

  template <class Jb>
  typename Jb::Wt *submitJob (Jb *job, bool autodelwt = false) {
    waitable *wt = job->getWaitable();
    lock();
    scl::jobs::job<waitable> *job_ = (scl::jobs::job<waitable> *)job;
    job_->autodelwt                = autodelwt;
    m_jobs.push (t_wjob (job_, wt));
    unlock();
    return (typename Jb::Wt *)wt;
  }

  waitable *submitJob (std::function<void (jobworker const &worker)> func,
    bool autodelwt = true);

  int workerCount() const;

  static void multithread (std::function<void (int, int)> func,
    int                                                   workers = 0);
};
} // namespace jobs
} // namespace scl
#endif
