#include <stdlib.h>
#include <math.h>
#include "fourierwindow.h"
#include "transform.h"
#include "transform_internal.h"

int ChirpAnalyse_Mode0(struct ANALYSER_STATE* state, double* data, size_t length) {
	int result = 0;
	size_t w;
	double* filteredr = 0;
	double* filteredi = 0;

	/* Initialise if needed */
	if (!state->resync_init) {
		state->resync_init = 1;
		/* Initialise a complex history buffer to fit a scan window */
		History_Buffer_Clear_Set_Params(&state->history, state->resync.window_width, 1);
	}
	
	/* Bandpass-filter the data */
	filteredr = (double*) malloc(sizeof(*filteredr) * length * 2);
	if (!filteredr) {
		state->errorcode = 0x0010;
		sprintf(state->error, "Failed to allocate filter buffer");
		goto exit;
	}
	filteredi = filteredr + length;
	FourierWindow(state->sgen->prelude_freq, data, filteredr, filteredi, state->history.position, length, state->sgen->samplerate);

	/* Append data to buffer */
	if (!History_Buffer_Append(&state->history, filteredr, filteredi, length)) {
		state->errorcode = 0x0020;
		sprintf(state->error, "Failed to add complex data to history buffer");
		goto exit;
	}

	/* Free filtered data */
	free(filteredr);
	filteredr = 0;

	/* Measure windows and add to list */
	for (w = state->resync.window_current_position; w < state->history.position - state->resync.window_width / 2; w += state->resync.window_spacing) {
		/* Analyse filtered window */
		if (!ChirpAnalyseFilteredWindow(state, w)) {
			state->errorcode = 0x0030;
			sprintf(state->error, "Failed to analyse prelude window");
			goto exit;
		}
	}

	/* Are we likely to have encountered the pulse by now? */
	if (state->history.position >= state->sgen->timing.drive_offset) {

		/* Union-find */
		if (!AnalyserUnionFind(state)) {
			state->errorcode = 0x0040;
			sprintf(state->error, "Failed to group windows");
			goto exit;
		}

		/* Do we have at least three windows? */
		if (state->resync.windows.count >=3 && state->history.position > (state->sgen->timing.prelude_offset + state->sgen->timing.prelude_samples / 2)) {
			struct ANALYSER_WINDOW* w0 = state->resync.windows.first;
			struct ANALYSER_WINDOW* w1 = w0->next;
			struct ANALYSER_WINDOW* w2 = w1->next;
			double threshold = state->resync.window_max * THRESHOLD_SCALE_FACTOR;
			size_t quarter_len = state->sgen->timing.prelude_samples / 4;
			/* Error in length of middle window, relative to expected pulse length */
			double error = fabs((w1->length - state->resync.window_width) * 1.0 - quarter_len * 1.0) / (1.0 * quarter_len);
			/* Is the error within the allowed tolerance, and is the middle window a peak? */
			if (w0->mean < threshold && w1->mean > threshold && w2->mean < threshold && error < state->resync.temporal_precision && w2->length > quarter_len / 2) {
				/* Store result for debugging purposes */
				state->resync.pulse = *w1;
				state->resync.pulse.next = 0;
				state->resync.drive_start_position = (w1->start + w1->end) / 2 + (quarter_len * 3) / 2;
				state->resync.chirp_start_position = state->resync.drive_start_position + state->sgen->timing.drive_padding_samples;
				/* Store error */
				state->result.time_error = error;		
				state->result.signal_to_noise = w1->mean / w0->mean;
				/* Clear window linked-list */
				ChirpAnalyseClearWindowList(state);
				/* Next mode */
				state->mode++;
				/* Return successful */
				result = 1;
				goto exit;
			}
		}
	
		/* Is it too late for us to expect our peak? */
		if (state->history.position > state->sgen->timing.prelude_offset + state->sgen->timing.prelude_samples + 2 * state->sgen->samplerate) {
			state->errorcode = 0x0050;
			sprintf(state->error, "Failed to locate prelude burst");
			goto exit;
		}
	}

	/* Clean up */
	result = 1;
	exit:
	free(filteredr);
	return result;
}
