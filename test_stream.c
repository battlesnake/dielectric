#include <Windows.h>
#include <MMSystem.h>
#include <stdlib.h>
#include <stdio.h>
#include "wave.h"
#include "transform.h"
#include "signalgen.h"
#include "graphing.h"

#define WAVEIN		0xface
#define WAVEOUT		0xdead

#define DEBUG		WAVEIN

#define SOLEN ( 6 )

size_t streamOut_callback(struct WAVEPARAMS* wp, size_t position, double* buffer, size_t count, void* param, struct WAVE_DOUBLE_STREAM_EXTRA* extra) {
	size_t i;
	double l;
	for (i = 0; i < count; i++) {
		double t = (i + position) * 1.0 / wp->samplerate;
		if (((int)((20.0 + (15000.0 * t/SOLEN)) * t)) & 1)
			buffer[i] = 1;
		else
			buffer[i] = -1;
	}
	l = (position + count) * 1.0 / wp->samplerate;
	if (l > SOLEN) {
		if (DEBUG == WAVEOUT)
			printf("\rProgress: %.1f/%.1d (%d%%); misses: %d", l, SOLEN, 100, extra->misses);
		return 0;
	}
	else {
		if (DEBUG == WAVEOUT)
			printf("\rProgress: %.1f/%.1d (%d%%); misses: %d", l, SOLEN, (int) (l*100.0 / SOLEN), extra->misses);
		return count;
	}
}

#define W (80)
size_t lastpos = 0;
void streamIn_callback(struct WAVEPARAMS* wp, size_t position, double* buffer, size_t count, void* param, struct WAVE_DOUBLE_STREAM_EXTRA* extra) {
	size_t i;
	double l;
	char graph[W+1];
	int bin = 1;
	double sum = 0;
	int error = 0;
	if (DEBUG == WAVEIN) {
		if (position > lastpos || extra->misses)
			puts("Packet(s) lost!");
		graph[W] = 0;
		graph[0] = '\r';
		for (i = 1; i < count; i++) {
			sum += buffer[i];
			if (i % (count / W) == 0) {
				if (sum > 0)
					graph[bin] = '+';
				else if (sum < 0)
					graph[bin] = '-';
				else {
					graph[bin] = '0';
					error++;
				}
				bin++;
				sum = 0;
			}
		}
		printf(graph);
		if (error)
			puts("");
	}
	l = (position + count) * 1.0 / wp->samplerate;
	extra->stop = l > SOLEN * 0.95;
	lastpos = position + count;
	return;
}

int StreamTest(struct WAVEPARAMS* w) {
	int wi, wo;
	w->length = 0;
	printf("\n");
	#pragma omp parallel sections
	{
		#pragma omp section
		wo = WaveOut_Double_Stream(w, &streamOut_callback, 0);
		#pragma omp section
		wi = WaveIn_Double_Stream(w, &streamIn_callback, 0);
	}
	return wi && wo;
}
