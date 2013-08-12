#include <stdlib.h>
#include <stdio.h>
#include "transform.h"

int AnalyserDump(struct ANALYSER_STATE* state, FILE* file) {
	size_t i;
	if (!file || !state)
		return 0;
	fprintf(file, "\"Electric spectroscope v1.0, (c) mark@battlesnake.co.uk 2013\"\n\n");
	fprintf(file, "fmin,%f,Hz\nfmax,%f,Hz\nchirp_length,%f,s\n\n", state->sgen->chirp_low, state->sgen->chirp_high, state->sgen->timing.sweep_samples * 1.0 / state->sgen->samplerate);
	fprintf(file, "time_error,%e,s\nnoise,%g,%%\nprelude_freq,%f\n\n", state->result.time_error, 100.0 / state->result.signal_to_noise, state->sgen->prelude_freq);
	fprintf(file, "%s,%s\n", "frequency", "intensity");
	for (i = 0; i < state->result.count; i++) {
		struct ANALYSER_RESULT* data = state->result.data + i;
		fprintf(file, "%f,%e\n", data->freq, data->value);
	}
	return 1;
}