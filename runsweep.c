#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include "wave.h"
#include "transform.h"
#include "mathutils.h"
#include "signed_t.h"
#include "graphing.h"
#include "signalgen.h"
#include "runsweep.h"

#ifndef _OPENMP
/* #error OpenMP must be enabled for this program! */
#endif

#define PRELUDE_BURST_LEN	(2)
#define PRELUDE_BURST_FREQ	(7000)
#define PUMP_SIGNAL_ATTEN	(1.0)
#define DRIVE_PADDING		(1.0)

#define ERROR_LEN 200

struct RUNSWEEP_STATE {
	struct SIGNALGEN_STATE* sgen;
	struct ANALYSER_STATE* anal;
	char error[ERROR_LEN];
	int stop;
};

/* Callback for streaming audio output */
size_t DRIVER_CALLBACK(struct WAVEPARAMS* wp, size_t position, double* buffer, size_t count, void* param, struct WAVE_DOUBLE_STREAM_EXTRA* extra) {
	struct RUNSWEEP_STATE* state = (struct RUNSWEEP_STATE*) param;
	struct SIGNALGEN_STATE* sgen = state->sgen;
	if (state->stop) {
		extra->stop = 1;
		return 0;
	}
	if (extra->misses) {
		sprintf(state->error, "Error: data lost in playback mechanism.");
		state->stop = 1;
		extra->fail = 1;
		return 0;
	}
	sgenGenerate(sgen, buffer, count);
	return count;
}

/* Callback for streaming audio input */
void ANALYSER_CALLBACK(struct WAVEPARAMS* wp, size_t position, double* buffer, size_t count, void* param, struct WAVE_DOUBLE_STREAM_EXTRA* extra) {
	struct RUNSWEEP_STATE* state = (struct RUNSWEEP_STATE*) param;
	struct ANALYSER_STATE* anal = state->anal;
	int analyserresult = AnalyserData(anal, buffer, count);
	if (state->stop) {
		extra->stop = 1;
		return;
	}
	if (extra->misses) {
		sprintf(state->error, "Error: data lost in recording mechanism.");
		state->stop = 1;
		extra->fail = 1;
		return;
	}
	if (!analyserresult) {
		sprintf(state->error, "Error: Analyser failed in mode %d with code %04x, message=\"%s\"\n", anal->mode, anal->errorcode, anal->error);
		state->stop = 1;
		extra->fail = 1;
		return;
	}
	if (anal->done) {
		extra->stop = 1;
		state->stop  = 1;
		return;
	}
	return;
}

/* Execute the duplex audio operation */
int SweepExecute(struct WAVEPARAMS* w, struct SIGNALGEN_STATE* sgen, struct ANALYSER_STATE* anal, char error[ERROR_LEN]) {
	struct RUNSWEEP_STATE state;
	HANDLE triggers[2];
	int result = 0;
	triggers[0] = CreateEvent(0, 1, 0, 0);
	triggers[1] = CreateEvent(0, 1, 0, 0);
	/* Prepare state information */
	state.error[0] = 0;
	state.stop = 0;
	state.anal = anal;
	state.sgen = sgen;
	/* OpenMP-assisted multithreading */
	#pragma omp parallel num_threads(2) reduction(|:result)
	{
		int tid = omp_get_thread_num();
		/* Set trigger from this thread */
		SetEvent(triggers[tid]);
		/* Wait for triggers from all threads */
		WaitForMultipleObjects(2, triggers, 1, INFINITE);
		/* Play thread */
		if (tid == 0) {
			if (!WaveOut_Double_Stream(w, &DRIVER_CALLBACK, (void*) &state))
				result |= 1;
		}
		/* Record thread */
		else if (tid ==	1) {
			if (!WaveIn_Double_Stream(w, &ANALYSER_CALLBACK, (void*) &state))
				result |= 2;
		}
		/* Abort all threads if this one failed. Synchronisation not necessary. */
		if (result)
			state.stop = 1;
	}
	/* Copy error message */
	if (state.stop)
		memcpy(error, state.error, ERROR_LEN);
	/* Free triggers */
	CloseHandle(triggers[0]);
	CloseHandle(triggers[1]);
	return result;
}

int RunSweep_CreateSignalGen(struct WAVEPARAMS* wp, double sweep_duration, double fmin, double fmax, struct SIGNALGEN_STATE** sgen) {
	puts("Initialising signal generator... ");
	*sgen = sgenInit(wp, PRELUDE_BURST_LEN, PRELUDE_BURST_FREQ, DRIVE_PADDING, sweep_duration, fmin, fmax, PUMP_SIGNAL_ATTEN);
	if (!sgen) {
		puts("Error: failed to initialise signal generator.");
		return 0;
	}
	wp->length += wp->samplerate;
	return 1;
}

int RunSweep_CreateAnalyser(struct SIGNALGEN_STATE* sgen, double sweep_duration, double sweep_r_rate, struct ANALYSER_STATE** anal) {
	size_t dp_octave = (size_t) (sgen->synth.chirp_logrange * 6);
	size_t dp_temporal = (size_t) (sweep_duration * 10);
	puts("Initialising analysis... ");
	*anal = AnalyserInit(sgen, 0.01, 0.0003, 0.03, sweep_r_rate / 1000, max(dp_octave, dp_temporal));
	if (!anal) {
		puts("Error: failed to initialise analyser.");
		return 0;
	}
	return 1;
}

void RunSweep_LowFreq_Warning(double sweep_r_rate, double fmin) {
	double min_freq_analysable = 3.0 * 1000.0 / sweep_r_rate;
	if (min_freq_analysable > fmin)
		printf("Note: frequencies below %.0f Hz should be ignored.  To reduce this limit, increase the chirp length or decrease the frequency range.\n", min_freq_analysable);
}

int RunSweep_Execute(struct WAVEPARAMS* wp, struct SIGNALGEN_STATE* sgen, struct ANALYSER_STATE* anal) {
	char error[ERROR_LEN];
	int wave_fail;
	puts("Initiating sweep... ");
	wave_fail = SweepExecute(wp, sgen, anal, error);
	/* Check for errors */
	if (wave_fail) {
		putchar('\n');
		puts(error);
		if (wave_fail & 0x01)
			puts("Error: failed to transmit signal.");
		if (wave_fail & 0x02)
			puts("Error: failed to receive signal.");
		puts("Error: sweep operation failed.");
		return 0;
	}
	return 1;
}

int RunSweep_DumpData(char* filename, struct ANALYSER_STATE* anal) {
	FILE* f;
	printf("Dumping the spectrum to file \"%s\"... ", filename);
	f = fopen(filename, "w");
	if (!f) {
		printf("Error: failed to open file.\n", filename);
		return 0;
	}
	AnalyserDump(anal, f);
	fclose(f);
	return 1;
}

/* Generate sweep signal, send it and record the response, dump the response, process the response, dump the result */
int RunSweep(struct WAVEPARAMS* wp, float fmin, float fmax, float sweep_duration, char* filename) {
	int result = 0;
	struct SIGNALGEN_STATE* sgen;
	struct ANALYSER_STATE* anal;
	char fname[MAX_PATH + 1];
	double seconds_per_octave = sweep_duration / (log(fmax / fmin) / log(2.0));
	
	if (!RunSweep_CreateSignalGen(wp, sweep_duration, fmin, fmax, &sgen))
		goto exit;

	if (!RunSweep_CreateAnalyser(sgen, sweep_duration, seconds_per_octave, &anal))
		goto exit;

	RunSweep_LowFreq_Warning(seconds_per_octave, fmin);

	if (!RunSweep_Execute(wp, sgen, anal))
		goto exit;

	sprintf(fname, "%s-x.csv", filename);
	if (!RunSweep_DumpData(fname, anal))
		goto exit;

	result = 1;
exit:
	if (!result && WaveLastError)
		printf("Error: mmresult=%d\n", WaveLastError);
	if (anal)
		AnalyserFree(anal);
	if (sgen)
		sgenFree(sgen);
	return result;
}
