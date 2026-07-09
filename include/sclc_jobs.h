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


/* sclc_jobs.h
 * adds async job queues
 */

#ifndef sclc_jobs_h
#define sclc_jobs_h

#ifdef __cplusplus
extern "C" {
#endif

#define SCL_JP_GENERAL 0
#define SCL_JP_IO      1

/**
 * @brief Creates a job pool with a specified max amount of threads.
 *
 * @return Returns the id of the created job pool. dont loose it.
 */
int scl_jcreatepool(int maxthreads);

/**
 * @brief Starts the job pool with the supplied id. Job pools will be started by
 * default.
 *
 * @param  id
 */
void scl_jstart(int id);

/**
 * @brief Pauses the job pool with the supplied id. Jobs being worked on will
 * be completed before returning.
 * @warning If a job does not exit while the job pool is paused, the thread
 * calling scl_jstop will be locked.
 *
 * @param  id
 */
void scl_jstop(int id);

/**
 * @brief Releases the job pool with the supplied id. Said id will become
 * invalid.
 *
 * @param  id
 */
void scl_jfree(int id);

/**
 * @brief Whether or not the job pool threads are currently working.
 *
 * @param  id  Job pool id
 * @return Returns true when jobs are currently being worked on.
 */
bool scl_jisworking(int id);

/**
 * @brief Waits for all job threads to be idle (scl_jisworking()==false).
 *
 * @param  timeout  Max wait time in seconds. Set to negative number for an
 * infite timeout.
 * @return  Returns true if the wait didnt timeout.
 */
bool scl_jwaitidle(int id, double timeout);

void scl_jsetmaxthreads(int id, int maxthreads);

void scl_jclearjobs(int id);


#ifdef __cplusplus
}
#endif
#endif
