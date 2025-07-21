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

/**
 * @brief Class used by multithreaded jobs to pass results to a possibly
 * syncronized environment.
 *
 */
class waitable {
  template <class Wt>
  friend class job;
  using _Waitable = bool;

 private:
  std::atomic_bool m_done;

 protected:
 public:
  waitable();

  /**
   * @brief Completes the waitable.
   *
   */
  void complete();

  /**
   * @brief Waits for this waitable to be marked completed.
   *
   * @param timeout  Max number of seconds to wait.
   * @return   True: Wait did not time out, False: Wait did time out.
   */
  bool wait(double timeout = -1);
};

class JobWorker;
class JobServer;

/**
 * @brief Job class. Used by JobServer and JobWorker to perform a task in a
 * multithreaded environment while still allowing syncronization.
 *
 * @tparam WtT  Type of waitable used by this job.
 */
template <class WtT>
class job {
  friend class JobServer;
  friend class JobWorker;

 private:
  bool                                 autodelwt    = false;
  static const typename WtT::_Waitable _is_waitable = 1;

 protected:
 public:
  using Wt       = WtT;
  virtual ~job() = default;

  /**
   * @brief Virtual method called to get a jobs unique waitable.
   *
   * @return   A NEW handle to this jobs waitable. It will be passed back to it
   * when doJob() is called.
   */
  virtual Wt  *getWaitable() const = 0;

  /**
   * @brief Virtual method called by workers, to check if a job can be taken.
   *
   * @param worker  Reference to the calling job worker.
   * @return   Whether or not this job can be taken.
   */
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

  /**
   * @return   Job server that owns this worker.
   */
  JobServer  &serv() const;

  /**
   * @brief  Syncs the job server (freezes job queue, and waits for all workers
   * to be idle), and calls given lambda function.
   *
   * @param func  Lambda function to call while synced.
   */
  void        sync(const std::function<void()> &func) const;

  /**
   * @return   This workers id.
   */
  int         id() const;

  /**
   * @return   Whether or not this worker is taking working.
   */
  bool        working() const;

  /**
   * @return   Whether or not this worker is completing a job.
   */
  bool        busy() const;

  static void work(JobWorker *inst);
};

/**
 * @brief Class that allows submission of jobs from a syncronized environment,
 * to a multithreaded environment.
 *
 */
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

  static int            ClampThreads(int threads);

 public:
  /**
   * @brief Construct a new Job Server object
   *
   * @param workers  Number of threads to multithread with, with a max of the
   * number of threads in the system.
   */
  JobServer(int workers = INT_MAX);
  ~JobServer();

  JobServer(const JobServer &)      = delete;
  JobServer &operator=(JobServer &) = delete;

  /**
   * @return Whether or not this job server is accepting jobs.
   */
  bool       is_working() const;

  /**
   * @brief Starts the job server, necessary for most operations.
   *
   */
  void       start();

  /**
   * @brief Allows job workers to poll for jobs at a slower rate.
   *
   * @param state  True: 1ms poll rate, False: 0.001ms poll rate.
   * @note Job servers default to fast polling rates (0.001ms).
   */
  void       slow(bool state = true);

  /**
   * @brief Waits for all workers to be idle.
   *
   * @param timeout  Max wait time in seconds. Defaults to -1 seconds
   * (infinite).
   * @return  True: Wait did not time out, False: Wait did time out.
   */
  bool       waitidle(double timeout = -1);

  /**
   * @brief Stops the job server, ignores any untaken jobs.
   *
   */
  void       stop();

  /**
   * @brief Set the lock bits of this server.
   *
   * @param bits  Bits to set.
   */
  void       setLockBits(size_t bits);

  /**
   * @brief Unset the lock bits of this server.
   *
   * @param bits  Bits to unset.
   */
  void       unsetLockBits(size_t bits);

  /**
   * @param bits  Bits to check.
   * @return   Whether or not this server has any of the given bits set.
   */
  bool       hasLockBits(size_t bits) const;

  /**
   * @brief Clears the job queue.
   *
   */
  void       clearjobs();

  /**
   * @brief  Syncs the job server (freezes job queue, and waits for all workers
   * to be idle), and calls given lambda function.
   *
   * @param func  Lambda function to call while synced.
   */
  void       sync(const std::function<void()> &func);

  /**
   * @brief Submits a job instance to the job server.
   *
   * @tparam Jb  Type of job.
   * @param job  Handle to a new job instance to be completed.
   * @param autodelwt  Whether or not to automatically delete the waitable
   * handle returned by this method.
   * @return  Waitable handle, with the waitable type of the job. Can be used to
   * wait for job completion, and get results.
   */
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

  /**
   * @brief Submits a lambda function to be worked on by the job server.
   *
   * @param  func  Lambda function to call.
   * @param  autodelwt  Whether the waitable should be automatically deleted
   * when the job is complete.
   * @return  Waitable handle. Only necessary if you need to wait for the job to
   * be complete.
   * @note  If autodelwt = false, you must free the waitable handle.
   */
  waitable   *submitJob(std::function<void(const JobWorker &worker)> func,
      bool autodelwt = true);

  /**
   * @return  Number of workers in this server.
   */
  int         workerCount() const;

  /**
   * @return  Number of logical processors (threads) of the system.
   */
  static int  GetNumThreads();

  /**
   * @brief  Multithreads a lambda function over a given number of threads.
   *
   * @param  func  Lambda function to be multithreaded.
   * @param  workers  Number of threads to multithread with, with a max of the
   * number of threads in the system.
   */
  static void Multithread(std::function<void(int id, int workers)> func,
    int workers = INT_MAX);
};
} // namespace jobs
} // namespace scl
#endif
