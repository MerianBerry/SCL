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
