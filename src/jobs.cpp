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


/*  jobs.cpp
 *  SCL work multithreading library
 */

#include <scl_jobs.hpp>
#include <scl_time.hpp>
#include <algorithm>

#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#else
#  include <unistd.h>
#endif

namespace scl {
namespace jobs {

waitable::waitable() {
  m_done = false;
}

waitable::waitable(waitable&& rhs) {
  m_done = rhs.m_done.load();
}

waitable& waitable::operator=(waitable&& rhs) {
  m_done = rhs.m_done.load();
  return *this;
}

void waitable::complete() {
  m_done = true;
}

void waitable::reset() {
  m_done = false;
}

bool waitable::status() const {
  return m_done.load();
}

bool waitable::wait(double timeout) {
  // This should be relatively safe. m_wdone is only set by waiting threads, and
  // isnt complex.
  return m_done ||
    waitUntil(
      [&]() {
        bool state = m_done;
        return state;
      },
      timeout);
}

funcJob::funcJob(std::function<void(const JobWorker& worker)> func)
    : m_func(func) {
}

waitable* funcJob::getWaitable() const {
  return new waitable;
}

void funcJob::doJob(waitable* waitable, const JobWorker& worker) {
  m_func(worker);
}

void JobWorker::quit() {
  m_working = false;
}

JobWorker::JobWorker(JobServer* serv, int id) {
  m_serv = serv;
  m_id = id;
  m_working = false;
  m_busy = false;
}

int JobWorker::id() const {
  return m_id;
}

JobServer& JobWorker::serv() const {
  return *m_serv;
}

void JobWorker::sync(const std::function<void()>& func) const {
  m_serv->sync(func);
}

bool JobWorker::working() const {
  return m_working;
}

bool JobWorker::busy() const {
  return m_busy;
}

void JobWorker::work(JobWorker* inst) {
  inst->m_working = true;
  JobServer* serv = inst->m_serv;
  do {
    JobServer::t_wjob wjob;
    waitUntil(
      [&]() {
        bool foundjob = serv->takeJob(wjob, *inst);
        return foundjob || !inst->working();
      },
      -1,
      SCL_JOBS_SLEEP(serv->m_slow));
    if(!inst->working())
      break;
    job<waitable>* job = wjob.first;
    waitable* wt = wjob.second;

    inst->m_busy = true;
    if(job) {
      job->doJob(wt, *inst);
      wt->complete();
      if(job->autodelwt)
        delete wt;
      delete job;
    }
    inst->m_busy = false;
  } while(inst->working());
}

bool JobServer::takeJob(t_wjob& wjob, const JobWorker& worker) {
  if(!m_working)
    return false;
  lock();
  auto avail = m_jobs.size();
  while(avail) {
    wjob = m_jobs.front();
    if(wjob.first->checkJob(worker)) {
      m_jobs.pop();
      break;
    }
    m_jobs.push(wjob);
    m_jobs.pop();
    avail--;
  }
  unlock();
  return !!avail;
}

int JobServer::GetNumThreads() {
  int n = 0;
#ifdef _WIN32
  SYSTEM_INFO si;
  GetSystemInfo(&si);
#  define sysconf(...) si.dwNumberOfProcessors
#  define _SC_NPROCESSORS_ONLN
#else
#endif
#ifdef _SC_NPROCESSORS_ONLN
  n = (int)sysconf(_SC_NPROCESSORS_ONLN);
  return n;
#endif
  return 0;
}

int JobServer::ClampThreads(int threads) {
  int max = GetNumThreads();
  if(max)
    return std::max(std::min(threads, max), 1);
  else
    return 0;
}

JobServer::JobServer(int workers) {
  int n = ClampThreads(workers);
  m_workers.reserve((size_t)n);
  m_nworkers = n;
  m_slow = false;
  m_working = false;

  for(int i = 0; i < n; i++) {
    m_workers.push_back(t_worker());
  }
}

JobServer::~JobServer() {
  stop();
}

bool JobServer::is_working() const {
  return m_working;
}

void JobServer::start() {
  if(!m_working) {
    m_working = true;
    lock();
    for(size_t i = 0; i < (size_t)m_nworkers; i++) {
      JobWorker* worker = new JobWorker(this, (int)i);
      std::thread t(JobWorker::work, worker);
      t.swap(m_workers[i].first);
      m_workers[i].second = worker;
      waitUntil([&]() {
        return worker->working();
      });
    }
    unlock();
  }
}

void JobServer::slow(bool state) {
  m_slow = state;
}

bool JobServer::waitidle(double timeout) {
  if(!m_working)
    return true;
  return waitUntil(
    [&]() {
      lock();
      bool cond = m_jobs.empty();
      for(auto& i : m_workers) {
        if(i.second->busy()) {
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
  if(m_working) {
    // tell all workers to quit, then join worker
    m_working = false;
    for(auto& i : m_workers) {
      i.second->quit();
      if(i.first.joinable())
        i.first.join();
      delete i.second;
    }
  }
}

void JobServer::setLockBits(size_t bits) {
  size_t b = m_lockBits;
  m_lockBits = b | bits;
}

void JobServer::unsetLockBits(size_t bits) {
  size_t b = m_lockBits;
  m_lockBits = b ^ bits;
}

bool JobServer::hasLockBits(size_t bits) const {
  size_t b = m_lockBits;
  return b & bits;
}

void JobServer::clearjobs() {
  lock();
  t_wjob wjob;
  while(!m_jobs.empty()) {
    wjob = m_jobs.front();
    m_jobs.pop();
    if(wjob.first->autodelwt)
      delete wjob.second;
    delete wjob.first;
  }
  unlock();
}

void JobServer::sync(const std::function<void()>& func) {
  if(!m_working)
    return;
  lock();
  func();
  unlock();
}

waitable* JobServer::submitJob(
  std::function<void(const JobWorker& worker)> func, bool autodelwt) {
  return &submitJob(new funcJob(func), autodelwt);
}

int JobServer::workerCount() const {
  return m_nworkers;
}

void JobServer::Multithread(
  std::function<void(int id, int workers)> func, int workers) {
  int n = ClampThreads(workers);
  std::vector<std::thread> w;
  for(int i = 0; i < n; i++)
    w.push_back(std::thread(func, i, n));
  for(auto& i : w) {
    if(i.joinable())
      i.join();
  }
}
} // namespace jobs
} // namespace scl
