#pragma once
#include "wave.h"
#ifdef __cplusplus
extern "C" {
#endif

#undef TEST
/*
#define TEST SweepEchoStreamTest
*/

#ifndef _DEBUG
#undef TEST
#endif

int StreamTest(struct WAVEPARAMS* w);
int EchoStreamTest(struct WAVEPARAMS* w);
int SweepEchoStreamTest(struct WAVEPARAMS* w);

#ifdef __cplusplus
}
#endif
