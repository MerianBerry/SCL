/* sclc_base.h
 * base definitions
 */

#ifndef sclc_base_h
#define sclc_base_h

#include <stdint.h>
#include <stddef.h>
#ifndef __cplusplus
#  include <stdbool.h>
#endif

#if defined(_MSC_VER)
#  include <BaseTsd.h>
#  include <malloc.h>
typedef SSIZE_T ssize_t;
#else
#  include <alloca.h>
#endif


#endif
