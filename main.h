#pragma once
#include "wave.h"
#ifdef __cplusplus
extern "C" {
#endif

int main(int, char**);
int GetWaveConfig(struct WAVEPARAMS*);
int GetSweepLen(float*);
int GetSweepRange(struct WAVEPARAMS*, float*, float*);
int MainLoop(struct WAVEPARAMS*, float, float, float);

#ifdef __cplusplus
}
#endif
