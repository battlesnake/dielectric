#pragma once
#include "wave.h"
#ifdef __cplusplus
extern "C" {
#endif

int MainLoop(struct WAVEPARAMS* w, float fmin, float fmax, float sweepduration);

#ifdef __cplusplus
}
#endif
