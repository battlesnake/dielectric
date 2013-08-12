#pragma once
#include "wave.h"
#ifdef __cplusplus
extern "C" {
#endif

int RunSweep(struct WAVEPARAMS* wp, float fmin, float fmax, float sweep_duration, char* filename);

#ifdef __cplusplus
}
#endif
