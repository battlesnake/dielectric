#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <math.h>
#include "wave.h"
#include "getdevices.h"
#include "test.h"
#include "main.h"

#define ln2 0.69314718055994530941723212145818

/*
 * Sample rates and bit depths to test in auto-configure
 * Maximum supported sample rate is chosen, then maximum supported bit depth at that sample rate
 */
#define NUM_RATES 4
#define NUM_DEPTHS 2
int rates[NUM_RATES] = {44100, 48000, 96000, 192000 };
int depths[NUM_DEPTHS] = { 1, 2 };

/* Entry point (nb. see 'cmain') */
int main(int argc, char** argv) {
	struct WAVEPARAMS w;
	float fmin, fmax;
	float sweepduration;
	float sweeprate;
	puts("Mark's dielectric spectroscopy program");
	puts("======================================\n");
	if (!GetInput(&w)) {
		puts("Error: no audio input devices found.");
		return 1;
	}
	if (!GetOutput(&w)) {
		puts("Error: no audio output devices found.");
		return 1;
	}
	if (!GetWaveConfig(&w)) {
		puts("Error: no compatible combination of sample rate and bit depth exists between input and output devices for single-channel audio.");
		return 1;
	}
	printf("Chosen parameters: sample rate = %d, bit depth = %d\n\n", w.samplerate, w.depth*8);
	/* Tests */
#ifdef TEST
	return TEST(&w);
#endif
	if (!GetSweepLen(&sweepduration)) {
		puts("Error: failed to get sweep length.");
		return 1;
	}
	if (!GetSweepRange(&w, &fmin, &fmax)) {
		puts("Error: failed to get sweep range.");
		return 1;
	}
	sweeprate = (float) (log(fmax / fmin) / (ln2 * sweepduration));
	if (sweeprate > 1)
		printf("Sweep rate: \t%.1f octaves/second\n", sweeprate);
	else
		printf("Sweep rate: \t%.1f second/octave\n", 1.0f / sweeprate);
	MainLoop(&w, fmin, fmax, sweepduration);
	if (_isatty(_fileno(stdin)))
		getchar();
	return 0;
}

/* Entry point for RELEASE build, which uses minimal CRT */
void cmain() {
	int result;
	result = main(0, 0);
	ExitProcess(result);
}

int GetWaveConfig(struct WAVEPARAMS* w) {
	int r, d;
	w->samplerate = 0;
	w->depth = 0;
	w->channels = 1;
	for (r = 0; r < NUM_RATES; r++) 
		for (d = 0; d < NUM_DEPTHS; d++) 
			if (WaveInTest(w->dev_in, rates[r], depths[d], 1) && WaveOutTest(w->dev_out, rates[r], depths[d], 1)) {
				w->samplerate = rates[r];
				w->depth = depths[d];
			}
	if (w->samplerate) {
		puts("");
		return 1;
	}
	else
		return 0;
}

int GetSweepLen(float* sweepduration) {
	do {
		printf("Duration (>5): \t");
		if (scanf("%f", sweepduration) < 1)
			return 0;
		if (*sweepduration < 5) {
			puts("Error: sweep length must be five seconds or longer.");
			continue;
		}
		break;
	} while (1);
	return 1;
}

#define FMIN 10
int GetSweepRange(struct WAVEPARAMS* w, float* fmin, float* fmax) {
	do {
		printf("F_min (>%d): \t", FMIN);
		if (scanf("%f", fmin) == 0) 
			return 0;
	} while (*fmin < FMIN);
	do {
		printf("F_max (<%d): \t", w->samplerate/2);
		if (scanf("%f", fmax) == 0) 
			return 0;
	} while (*fmax > w->samplerate/2);
	return *fmax > *fmin;
}
