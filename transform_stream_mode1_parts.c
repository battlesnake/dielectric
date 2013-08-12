#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include "mathutils.h"
#include "history_buffer.h"
#include "transform.h"
#include "transform_internal.h"

/*double hypot(double x, double y) {
    double t;
    x = fabs(x);
    y = fabs(y);
    t = min(x, y);
    x = max(x, y);
    t = t / x;
    return x * sqrtf(1 + t * t);
}*/

int ChirpAnalyseAppendDatapoint(struct ANALYSER_STATE* state,
	struct ANALYSER_MODE1_DATAPT* datapt) {
	struct ANALYSER_RESULTS* list;
	struct ANALYSER_RESULT* data;
	int result = 0;
	
	list = &state->result;

	/* Extend the list if needed */
	if (list->count == list->capacity) {
		size_t newcapacity = list->capacity + 64;
		struct ANALYSER_RESULT* larger;
		larger = (struct ANALYSER_RESULT*) realloc((void*) list->data,
			sizeof(*list->data) * newcapacity);
		if (!larger)
			goto exit;
		list->data = larger;
		list->capacity = newcapacity;
	}

	/* Add the new values to the list */
	data = list->data + list->count;
	list->count++;
	data->idx = datapt->idx;
	data->freq = datapt->freq;
	data->value = datapt->value;

	/* Clean up */
	result = 1;
	exit:
	return result;
}

int ChirpAnalyseReadDatapoint(struct ANALYSER_STATE* state, size_t pointindex, struct ANALYSER_MODE1_DATAPT* datapt) {
	/* Set datapt->freq = -1 and return non-zero when no error occurs, but no datapoint was available */
	double* data;
	size_t datapt_rectime, datapt_gentime, datapt_offset;
	signed long long i_omp;
	size_t window_width;
	size_t window_halfwidth;
	size_t start, length;
	double sum_r, sum_i, sum_weight;
	double omega, value, freq, omegapersample;
	int result = 0;

	/* Get index of this point in recorded and generated buffers */
	datapt_offset = (size_t) floor(state->sgen->timing.sweep_samples * (pointindex * 1.0 / (state->decode.requested_datapoints - 1)));
	datapt_rectime = state->resync.chirp_start_position + datapt_offset;
	datapt_gentime = state->sgen->timing.sweep_offset + datapt_offset;

	/* Frequency at centre */
	omega = sgenChirpOmega(state->sgen, datapt_gentime);
	freq = omega / (2 * PI);
	omegapersample = omega / state->sgen->samplerate;

	/* Width of window */
	window_width = state->decode.fft_window_width;
	window_halfwidth = window_width / 2;

	/* Range check: ensure that we have a full window */
	if (datapt_rectime < window_halfwidth)
		goto exit;
	start = datapt_rectime - window_halfwidth;
	length = window_width + 1;

	/* Sanity check, shouldn't be necessary */
	if (start < state->history.start_offset) {
		state->errorcode = 0x0180;
		sprintf(state->error, "Missed a data point somehow");
		goto exit;
	}

	/* Pointer to start of gaussian */
	if (!History_Buffer_Get_Data(&state->history, start, length, &data, 0)) {
		result = 1;
		datapt->freq = -1;
		goto exit;
	}

	/* Windowed Fourier transform */
	sum_r = 0;
	sum_i = 0;
	sum_weight = 0;
	#pragma omp parallel for reduction(+:sum_r) reduction(+:sum_i) reduction(+:sum_weight)
	for (i_omp = 0; i_omp < (signed long long) length; i_omp++) {
		size_t i = (size_t) i_omp;
		/* Sinc window
		double windowparam = 2 * ((long long) i - (long long) window_halfwidth) * PI / window_width;
		double windowvalue = (fabs(windowparam) < 1e-2) ? 1.0 : (sin(windowparam) / windowparam);/**/
		/* Boxcar window
		double windowvalue = 1.0;/**/
		/* Gaussian window 
		double windowparam = ((long long) i - (long long) window_halfwidth) * 1.0 / window_halfwidth;
		double windowvalue = exp(-sqr(2*windowparam));/**/
		/* Hann window 
		double windowvalue = sqr(sin(i * PI / window_width));/**/
		/* Blackman window */
		double windowvalue = 0.42 + -0.5*cos(2*i*PI/window_width) + 0.08*cos(4*i*PI/window_width);/**/
		/* Fourier transform */
		double theta = i * omegapersample;
		sum_r += windowvalue * data[i] * cos(theta);
		sum_i += windowvalue * data[i] * sin(theta);
		/* 
		 * Accumulate weight from envelope.  Periodic sinusoids form an
		 * orthonormal basis, so no weight factor is needed for fourier
		 * transform.
		 */
		sum_weight += windowvalue;
	}

	/* Adjust weight by measured attenuation of timing burst */
	sum_weight *= state->sgen->drive_level * state->resync.pulse.mean;

	/* Create data point and store */
	value = _hypot(sum_r, sum_i) / sum_weight;
	datapt->idx = datapt_rectime;
	datapt->freq = freq;
	datapt->value  = value;
	
	/* Clean up */
	result = 1;
	exit:
	return result;
}
