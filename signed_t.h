#pragma once
#include <stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _LP64
typedef signed long long signed_t;
#else
typedef signed long signed_t;
#endif

#ifdef __cplusplus
}
#endif
