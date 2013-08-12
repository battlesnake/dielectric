#include <Windows.h>
#include <MMSystem.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "mathutils.h"
#include "wave.h"
#include "transform.h"
#include "signalgen.h"
#include "graphing.h"
#include "test.h"

#define DPTS 200
/*
#define USE_TEST_SIGNAL
#define DUMP_SGEN
#define DUMP_ANAL
*/
#define PRINT_ANAL

#define GRAPHWIDTH ((DPTS < 160) ? DPTS : 160)

int SweepEchoStreamTest(struct WAVEPARAMS* w) {
	size_t i, j;
	struct SIGNALGEN_STATE* sgen;
	struct ANALYSER_STATE* anal;
	double buffer[MULTIBLOCK_BUFFERS_LENGTH];
	double L[DPTS];
	double I[DPTS];
	FILE* f;
	FILE* r;

	printf("\n");

	sgen = sgenInit(w, 1.0, 7000, 1.0, 1200.0, 30, 50000, 0.5);
	anal = AnalyserInit(sgen, 0.01, 0.0003, 0.01, 0.1, DPTS);

	printf("Total length: %llu\n", (long long unsigned) w->length);
	printf("Sweep range: %.0f-%.0f\n", sgen->chirp_low, sgen->chirp_high);

#ifdef DUMP_SGEN
	r = fopen("sesdump_sgen.csv", "w");
	if (r)
		fprintf(r, "t,I\n");
#else
	r = 0;
#endif
	for (i = 0; i < sgen->timing.length; i += MULTIBLOCK_BUFFERS_LENGTH) {
#ifdef USE_TEST_SIGNAL
		if (i >= sgen->timing.drive_offset) {
			for (j = 0; j < MULTIBLOCK_BUFFERS_LENGTH; j++)
				buffer[j] = (0.3*sin(2.0 * PI * 2000.0 * (i + j) * 1.0 / sgen->samplerate))
					  + (0.3*sin(2.0 * PI * 3000.0 * (i + j) * 1.0 / sgen->samplerate))
					  + (0.3*sin(2.0 * PI * 5000.0 * (i + j) * 1.0 / sgen->samplerate));
		}
		else
			sgenGenerate(sgen, buffer, MULTIBLOCK_BUFFERS_LENGTH);
#else
		sgenGenerate(sgen, buffer, MULTIBLOCK_BUFFERS_LENGTH);
#endif
		if (!AnalyserData(anal, buffer, MULTIBLOCK_BUFFERS_LENGTH)) {
			printf("Error: Analyser failed in mode %d with code %04x, message=%s\n", anal->mode, anal->errorcode, anal->error);
			break;
		}
		if (r)
			for (j = 0; j < MULTIBLOCK_BUFFERS_LENGTH; j++)
				fprintf(r, "%f,%f\n", (i + j) * 1.0 / sgen->samplerate, buffer[j]);
	}
	if (r)
		fclose(r);
	else
		puts("Failed to dump signal data");

#ifdef DUMP_ANAL
	f = fopen("sesdump_anal.csv", "w");
	if (f) {
		fprintf(f, "a_drive_start,%f,s\n", anal->resync.drive_start_position * 1.0 / sgen->samplerate);
		fprintf(f, "a_chirp_start,%f,s\n", anal->resync.chirp_start_position * 1.0 / sgen->samplerate);
		fprintf(f, "\n");
		AnalyserDump(anal, f);
		fclose(f);
	}
	else
		puts("Failed to dump analysis data");
#else
	f = 0;
#endif
	
#ifdef PRINT_ANAL
	AnalyserDump(anal, stdout);
#endif

	printf("Graphing %d/%d data points for (a) linear magnitude, (b) logarithmic magnitude\n", anal->result.count, DPTS);
	printf("Temporal error = %f\n", anal->result.time_error);

	for (i = 0; i < anal->result.count; i++) {
		double v = anal->result.data[i].value;
		I[i] = v;
		if (v < 1e-3)
			v = 1e-3;
		L[i] = log10(v);
	}

	puts("a)");
	GraphSeries(I, anal->result.count, GRAPHWIDTH, 10, 0, 0, GRAPHSERIES_USE_YMIN | GRAPHSERIES_FILL_RANGE);
	puts("b)");
	GraphSeries(L, anal->result.count, GRAPHWIDTH, 10, -3, 0, GRAPHSERIES_USE_YMIN | GRAPHSERIES_USE_YMAX | GRAPHSERIES_FILL_RANGE | GRAPHSERIES_HIDE_MEAN | GRAPHSERIES_HIDE_EXTREMA);

	AnalyserFree(anal);
	sgenFree(sgen);

	puts("DONE!");

	getchar();

	return 1;
}
