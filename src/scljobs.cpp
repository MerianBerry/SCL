/*  scljobs.cpp
 *  SCL work multithreading library
 */

#include "scljobs.hpp"
#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  ifdef min
#    undef min
#  endif
#else
#  include <unistd.h>
#endif

namespace scl {
namespace jobs {

waitable::waitable() {
  m_done = false;
}

void waitable::complete() {
  m_done = true;
}

bool waitable::wait (double timeout) {
  // This should be relatively safe. m_wdone is only set by waiting threads, and
  // isnt complex.
  return m_done || waitUntil (
                     [&]() {
                       bool state = m_done;
                       return state;
                     },
                     timeout);
}

funcJob::funcJob (std::function<void (const JobWorker &worker)> func)
    : m_func (func) {
}

waitable *funcJob::getWaitable() const {
  return new waitable;
}

void funcJob::doJob (waitable *waitable, const JobWorker &worker) {
  m_func (worker);
}

void JobWorker::quit() {
  m_working = false;
}

JobWorker::JobWorker (JobServer *serv, int id) {
  m_serv    = serv;
  m_id      = id;
  m_working = false;
  m_busy    = false;
}

int JobWorker::id() const {
  return m_id;
}

void JobWorker::sync (const std::function<void()> &func) const {
  m_serv->sync (func);
}

bool JobWorker::working() const {
  return m_working;
}

bool JobWorker::busy() const {
  return m_busy;
}

void JobWorker::work (JobWorker *inst) {
  inst->m_working = true;
  JobServer *serv = inst->m_serv;
  do {
    JobServer::t_wjob wjob;
    waitUntil (
      [&]() {
        bool foundjob = serv->takeJob (wjob);
        return foundjob || !inst->working();
      },
      -1,
      SCL_JOBS_SLEEP (serv->m_slow));
    if (!inst->working())
      break;
    job<waitable> *job = wjob.first;
    waitable      *wt  = wjob.second;

    inst->m_busy = true;
    if (job) {
      job->doJob (wt, *inst);
      wt->complete();
      if (job->autodelwt)
        delete wt;
      delete job;
    }
    inst->m_busy = false;
  } while (inst->working());
}

int JobServer::getnthreads (int threads) {
  if (threads <= 0)
    threads = 0x7fffffff;
  int n = 0;
#ifdef _WIN32
  SYSTEM_INFO si;
  GetSystemInfo (&si);
#  define sysconf(...) si.dwNumberOfProcessors
#  define _SC_NPROCESSORS_ONLN
#else
#endif
#ifdef _SC_NPROCESSORS_ONLN
  n = sysconf (_SC_NPROCESSORS_ONLN);
  return std::min (n, threads);
#endif
  return 0;
}

bool JobServer::takeJob (t_wjob &wjob) {
  if (!m_working)
    return false;
  lock();
  bool avail = !m_jobs.empty();
  if (avail) {
    wjob = m_jobs.front();
    m_jobs.pop();
  }
  unlock();
  return avail;
}

JobServer::JobServer (int workers) {
  int n = getnthreads (workers);
  m_workers.reserve (n);
  m_nworkers = n;
  m_slow     = false;
  m_working  = false;

  for (int i = 0; i < n; i++) {
    m_workers.push_back (t_worker());
  }
}

JobServer::~JobServer() {
  stop();
}

void JobServer::start() {
  if (!m_working) {
    m_working = true;
    lock();
    for (int i = 0; i < m_nworkers; i++) {
      JobWorker  *worker = new JobWorker (this, i);
      std::thread t (JobWorker::work, worker);
      t.swap (m_workers[i].first);
      m_workers[i].second = worker;
      waitUntil ([&]() {
        return worker->working();
      });
    }
    unlock();
  }
}

void JobServer::slow (bool state) {
  m_slow = state;
}

bool JobServer::waitidle (double timeout) {
  if (!m_working)
    return true;
  return waitUntil (
    [&]() {
      lock();
      bool cond = m_jobs.empty();
      for (auto &i : m_workers) {
        if (i.second->busy()) {
          cond = false;
          continue;
        }
      }
      unlock();
      return cond;
    },
    timeout,
    SCL_JOBS_SLOW_SLEEP);
}

void JobServer::stop() {
  if (m_working) {
    // tell all workers to quit, then join worker
    m_working = false;
    for (auto &i : m_workers) {
      i.second->quit();
      if (i.first.joinable())
        i.first.join();
      delete i.second;
    }
  }
}

void JobServer::clearjobs() {
  lock();
  t_wjob wjob;
  while (!m_jobs.empty()) {
    wjob = m_jobs.front();
    m_jobs.pop();
    if (wjob.first->autodelwt)
      delete wjob.second;
    delete wjob.first;
  }
  unlock();
}

void JobServer::sync (const std::function<void()> &func) {
  if (!m_working)
    return;
  lock();
  func();
  unlock();
}

waitable *JobServer::submitJob (
  std::function<void (const JobWorker &worker)> func, bool autodelwt) {
  return submitJob (new funcJob (func), autodelwt);
}

int JobServer::workerCount() const {
  return m_nworkers;
}

void JobServer::multithread (std::function<void (int, int)> func, int workers) {
  int                      n = getnthreads (workers);
  std::vector<std::thread> w;
  for (int i = 0; i < n; i++)
    w.push_back (std::thread (func, i, n));
  for (auto &i : w) {
    if (i.joinable())
      i.join();
  }
}
} // namespace jobs
} // namespace scl