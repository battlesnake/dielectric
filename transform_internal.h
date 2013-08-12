#pragma once
#include "transform.h"
#ifdef __cplusplus
extern "C" {
#endif

#define THRESHOLD_SCALE_FACTOR		(0.5)

/* Mode blocks */
int ChirpAnalyse_Mode0(struct ANALYSER_STATE* state, double* data,
		size_t length);
int ChirpAnalyse_Mode1(struct ANALYSER_STATE* state, double* data,
		size_t length);

/* Mode-0 private routines */
int AnalyserUnionFind(struct ANALYSER_STATE* state);
int ChirpAnalyseAppendFilteredWindow(struct ANALYSER_STATE* state,
	size_t centre, double ms);
int ChirpAnalyseFilteredWindow(struct ANALYSER_STATE* state, size_t centre);
void ChirpAnalyseClearWindowList(struct ANALYSER_STATE* state);

/* Mode-1 private routines */
struct ANALYSER_MODE1_DATAPT {
	size_t idx;
	double freq;
	double value;
};
int ChirpAnalyseAppendRawWindow(struct ANALYSER_STATE* state, size_t centre,
	double ms);
int ChirpAnalyseAppendDatapoint(struct ANALYSER_STATE* state,
	struct ANALYSER_MODE1_DATAPT* datapt);
int ChirpAnalyseReadDatapoint(struct ANALYSER_STATE* state, size_t pointindex,
	struct ANALYSER_MODE1_DATAPT* datapt);

#ifdef __cplusplus
}
#endif
