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

#ifndef SCLAPI
#  if defined(_MSC_VER)
#    ifdef SCL_DLL
#      define SCLAPI __declspec(dllexport)
#    else
#      define SCLAPI
#    endif
#  else
#    ifdef SCL_DLL
#      define SCLAPI __attribute__((visibility("default")))
#    else
#      define SCLAPI
#    endif

#  endif
#endif


#endif
