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

#endif

namespace scl {
namespace jobs {

void waitable::complete() {
  lock();
  m_done = true;
  unlock();
}

bool waitable::wait (double timeout) {
  return waitUntil (
    [&]() {
      lock();
      bool state = m_done;
      unlock();
      return state;
    },
    timeout);
}

funcJob::funcJob (std::function<void (jobworker const &worker)> func)
    : m_func (func) {
}

waitable *funcJob::getWaitable() const {
  return new waitable;
}

int funcJob::doJob (waitable *waitable, jobworker const &worker) {
  m_func (worker);
  return 0;
}

void jobworker::quit() {
  m_working = false;
}

jobworker::jobworker (jobserver *serv, int id) {
  m_serv    = serv;
  m_id      = id;
  m_working = false;
}

int jobworker::id() const {
  return m_id;
}

void jobworker::sync (std::function<void()> const &func) const {
  m_serv->sync (func);
}

bool jobworker::working() const {
  return m_working;
}

bool jobworker::busy() const {
  return m_busy;
}

void jobworker::work (jobworker *inst) {
  inst->m_working = true;
  jobserver *serv = inst->m_serv;
  do {
    jobserver::t_wjob wjob;
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

    if (job) {
      inst->m_busy = true;
      job->doJob (wt, *inst);
      wt->complete();
      if (job->autodelwt)
        delete wt;
      delete job;
      inst->m_busy = false;
    }
  } while (inst->working());
}

int jobserver::getNWorkers (int workers) {
  if (workers <= 0)
    workers = 0x7fffffff;
#ifdef _WIN32
  SYSTEM_INFO si;
  GetSystemInfo (&si);
  return std::min ((int)si.dwNumberOfProcessors, workers);
#else
#  error "Missing unix implementation of getNWorkers"
  return 0;
#endif
}

bool jobserver::takeJob (t_wjob &wjob) {
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

jobserver::jobserver (int workers) {
  int n = getNWorkers (workers);
  m_workers.reserve (n);
  m_nworkers = n;
  m_slow     = false;
  m_working  = false;

  for (int i = 0; i < n; i++) {
    m_workers.push_back (t_worker());
  }
}

jobserver::~jobserver() {
  stop();
}

void jobserver::start() {
  if (!m_working) {
    m_working = true;
    lock();
    for (int i = 0; i < m_nworkers; i++) {
      jobworker  *worker = new jobworker (this, i);
      std::thread t (jobworker::work, worker);
      t.swap (m_workers[i].first);
      m_workers[i].second = worker;
      waitUntil ([&]() {
        return worker->working();
      });
    }
    unlock();
  }
}

void jobserver::slow (bool state) {
  m_slow = state;
}

bool jobserver::waitidle (double timeout) {
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

void jobserver::stop() {
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

void jobserver::sync (std::function<void()> const &func) {
  if (!m_working)
    return;
  lock();
  func();
  unlock();
}

waitable *jobserver::submitJob (
  std::function<void (jobworker const &worker)> func, bool autodelwt) {
  return submitJob (new funcJob (func), autodelwt);
}

int jobserver::workerCount() const {
  return m_nworkers;
}

void jobserver::multithread (std::function<void (int, int)> func, int workers) {
  int                      n = getNWorkers (workers);
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