/* sclc_time.h
 *
 */

#ifndef sclc_time_h
#define sclc_time_h

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Resets the output of sclc_clock(), making current time epoch.
 *
 */
void scl_resetclock();

/**
 * @note You can use scl_resetclock() to control this function's epoch.
 *
 * @return   Seconds since epoch.
 */
double scl_clock();

/**
 * @brief Makes this thread sleep for a given amount of milliseconds.
 *
 * @param sleemms  Number of milliseconds to sleep for.
 */
void scl_waitms(double ms);

#ifdef __cplusplus
}
#endif
#endif
