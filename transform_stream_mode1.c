#include <math.h>
#include "transform.h"
#include "transform_internal.h"
#include "mathutils.h"

int ChirpAnalyse_Mode1(struct ANALYSER_STATE* state, double* data, size_t length) { 
	int result = 0;
	size_t i;
	size_t idx;
	size_t current_datapt;
	
	/* Initialise if needed */
	if (!state->decode_init) {
		state->decode_init = 1;
		/* Initialise a real history buffer to fit a fourier-transform window */
		History_Buffer_Clear_Set_Params(&state->history, state->decode.fft_window_width + 1, 0);
	}
	/* Append data to history buffer */
	if (!History_Buffer_Append(&state->history, data, 0, length)) {
		state->errorcode = 0x1010;
		sprintf(state->error, "Failed to add data to history buffer");
		goto exit;
	}

	/* Have we reached the start of the chirp yet? */
	if (state->history.position < state->resync.chirp_start_position)
		goto skip;

	/* Get current index within chirp */
	idx = state->history.position - state->resync.chirp_start_position;

	/* Index of current data point */
	current_datapt = (size_t) floor((state->decode.requested_datapoints - 1) * (idx * 1.0 / state->sgen->timing.sweep_samples));

	/* Have we advanced a data point (or just started)?  Log data if so. */
	for (i = state->result.count; i <= current_datapt; i++) {
		struct ANALYSER_MODE1_DATAPT datapt;
		/* Extract a data point */
		if (!ChirpAnalyseReadDatapoint(state, i, &datapt))
			goto exit;
		/* No error but no datapoint either */
		if (datapt.freq < 0)
			goto skip;
		/* Add the data point to the results list */
		if (!ChirpAnalyseAppendDatapoint(state, &datapt))
			goto exit;
		/* If we have the requested number of data points, go to next mode */
		if (state->result.count >= state->decode.requested_datapoints) {
			state->mode++;
			goto skip;
		}
	}

	/* Clean up */
	skip:
	result = 1;
	exit:
	return result;
}
