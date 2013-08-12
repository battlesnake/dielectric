#include <stdlib.h>
#include "transform.h"
#include "transform_internal.h"

struct ANALYSER_STATE* AnalyserInit(struct SIGNALGEN_STATE* sgen,
		double resync_window_size,
		double resync_window_spacing, double resync_temporal_precision,
		double decode_fft_window_width,
		size_t datapts) {
	struct ANALYSER_STATE* state = (struct ANALYSER_STATE*) malloc(sizeof(*state));
	int samplerate = sgen->samplerate;
	if (!state)
		goto error;
	if (datapts < 2)
		goto error;
	/* Initialise analyser state structure */
	memset(state, 0, sizeof(*state));
	state->sgen = sgen;
	state->resync.window_width = (size_t) (resync_window_size * samplerate);
	state->resync.window_spacing = (size_t) (resync_window_spacing * samplerate);
	state->resync.temporal_precision = resync_temporal_precision;
	state->decode.fft_window_width = (size_t) (decode_fft_window_width * samplerate);
	state->decode.requested_datapoints = datapts;
	state->result.data = (struct ANALYSER_RESULT*)
		malloc(sizeof(*state->result.data) * datapts);
	if (!state->result.data)
		goto error;

	/* Success: return state record */
	return state;

	/* Failed: free state record and return null */
	error:
	AnalyserFree(state);
	return 0;
}

void AnalyserFreeLinkedList(struct ANALYSER_WINDOW* first) {
	/* Free a window linked-list */
	struct ANALYSER_WINDOW* next;
	while (first) {
		next = first->next;
		free(first);
		first = next;
	}
}

void AnalyserFree(struct ANALYSER_STATE* state) {
	/* Sanity check */
	if (state) {
		/* Free resync window linked-lists */
		AnalyserFreeLinkedList(state->resync.windows.first);
		/* Free history buffer(s) */
		free(state->history.re);
		free(state->history.im);
		/* Free decode results buffer */
		free(state->result.data);
		free(state);
	}
}

int AnalyserData(struct ANALYSER_STATE* state, double* data, size_t length) {
	/* Sanity checks */
	if (!length)
		return 1;
 	if (!state || !data)
		return 0;
	/* See header file for description of modes */
	if (state->mode == 0) 
		return ChirpAnalyse_Mode0(state, data, length);
	else if (state->mode == 1)
		return ChirpAnalyse_Mode1(state, data, length);
	else {
		state->done = 1;
		return 1;
	}
}
