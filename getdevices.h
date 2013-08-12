#pragma once
#include "wave.h"
#ifdef __cplusplus
extern "C" {
#endif

int GetInput(struct WAVEPARAMS*);
int GetOutput(struct WAVEPARAMS*);

#ifdef __cplusplus
}
#endif
