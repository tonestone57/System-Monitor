#ifndef _OS_H
#define _OS_H

#include <stdint.h>

#ifndef B_OK
#define B_OK 0
#endif

#ifndef B_NO_MEMORY
#define B_NO_MEMORY -1
#endif

typedef int32_t status_t;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;
typedef int64_t bigtime_t;

#endif // _OS_H
