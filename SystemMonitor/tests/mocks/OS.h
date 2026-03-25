#ifndef MOCK_OS_H
#define MOCK_OS_H

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
typedef uint64_t uint64;
typedef long long int64;
typedef long long bigtime_t;

#endif
