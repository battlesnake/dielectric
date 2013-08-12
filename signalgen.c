#ifdef _DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>
#include <math.h>
#include "mathutils.h"
#include "wave.h"
#include "mathutils.h"
#include "signalgen.h"
#include "signed_t.h"

struct SIGNALGEN_STATE* sgenInit(struct WAVEPARAMS* wp, double prelude_len,
		double prelude_freq, double drive_padding, double chirp_len,
		double chirp_low, double chirp_high, double drive_level) {
	int samplerate;
	double duration;

	/* Allocate */
	struct SIGNALGEN_STATE* state = (struct SIGNALGEN_STATE*) malloc(sizeof(*state));
	if (!state)
		return 0;
	
	/* Calculate duration and set length field of wave params accordingly */
	duration = prelude_len + chirp_len + 2 * drive_padding;
	samplerate = wp->samplerate;
	wp->length = (size_t) (duration * samplerate);

	/* Store generator parameters */
	state->samplerate = samplerate;
	state->prelude_len = prelude_len;
	state->prelude_freq = prelude_freq;
	state->drive_len = chirp_len + 2 * drive_padding;
	state->chirp_low = chirp_low;
	state->chirp_high = chirp_high;
	state->drive_level = drive_level;
	state->duration = duration;

	/* Synth parameters */
	state->synth.prelude_omega = 2.0 * PI * prelude_freq;
	state->synth.chirp_omega_l = 2 * PI * chirp_low;
	state->synth.chirp_omega_h = 2 * PI * chirp_high;
	state->synth.chirp_logrange = log(chirp_high / chirp_low);
	state->synth.chirp_logstart_omega = log(2 * PI * chirp_low);
	state->synth.chirp_phase = 0;

	/* Calculate stream length */
	state->timing.position = 0;
	state->timing.length = (size_t) (duration * samplerate);
	state->timing.prelude_offset = 0;
	state->timing.prelude_samples = (size_t) (prelude_len * samplerate);
	state->timing.drive_offset = state->timing.prelude_offset + state->timing.prelude_samples;
	state->timing.drive_samples = (size_t) (state->drive_len * samplerate);
	state->timing.drive_padding_samples = (size_t) (drive_padding * samplerate);
	state->timing.sweep_offset = state->timing.drive_offset + state->timing.drive_padding_samples;
	state->timing.sweep_samples = (size_t) (chirp_len * samplerate);

	/* Just for fun */
	state->octavespersecond = samplerate * state->synth.chirp_logrange / (LN2 * state->timing.sweep_samples);

	/* Sanity checks */
	if (state->timing.drive_samples < 2 * state->timing.drive_padding_samples) {
		sgenFree(state);
		return 0;
	}
	if (chirp_low <= 0 || chirp_high <= 0) {
		sgenFree(state);
		return 0;
	}

	return state;
}

void sgenGenerate(struct SIGNALGEN_STATE* state, double* buffer, size_t samples) {
	size_t i, idx;
	double value;

	/* Fill buffer */
	for (i = 0; i < samples; i++) {
		/* Absolute index */
		idx = state->timing.position + i;

		/* Unknown - pre */
		if (idx < state->timing.prelude_offset) {
			value = 0;
		}

		/* Prelude */
		if (idx >= state->timing.prelude_offset && idx < state->timing.prelude_offset + state->timing.prelude_samples) {
			/* Relative and fractional time */
			size_t rel = idx - state->timing.prelude_offset;
			double t = rel * 1.0 / state->timing.prelude_samples;
			/* Third quarter contains timing burst, the rest of the prelude is empty */
			if (t >= 0.5 && t <= 0.75)
				value = sin(state->synth.prelude_omega * rel * 1.0 / state->samplerate);
			else
				value = 0;
		}

		/* Drive */
		else if (idx >= state->timing.drive_offset && idx < state->timing.drive_offset + state->timing.drive_samples) {
			state->synth.chirp_phase += sgenChirpOmega(state, idx) / state->samplerate;
			value = state->drive_level * sin(state->synth.chirp_phase);
		}
		/* Unknown - post */
		else {
			value = 0;
		}

		/* Write to buffer */
		buffer[i] = value;
	}

	/* Update stream position */
	state->timing.position += samples;

}

double sgenChirpOmega(struct SIGNALGEN_STATE* state, size_t index) {
	/* Logarithmic frequency sweep */
	/*
	 * \omega(t_f) = \omega_0^{1 - t_f} * \omega_1^{t_f}
	 *             = \omega_0 * \frac{\omega_1}{\omega_0}^{t_f}
	 *             = e^{ln\omega_0 + t_f * ln\frac{\omega_1}{\omega_0}}
	 */
	return state->synth.chirp_omega_l * 
		pow(state->synth.chirp_omega_h / state->synth.chirp_omega_l, 
		((long long) index - (long long) state->timing.sweep_offset) * 1.0 / state->timing.sweep_samples);
}

void sgenFree(struct SIGNALGEN_STATE* state) {
	free(state);
}
