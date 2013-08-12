#pragma once
#include "wave.h"
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Signal generator
 *
 * Signal consists of a prelude (used to resync timing with the recorded data),
 * a chirp signal (used to analyse the frequency response of the subject) and
 * a lead-out (for timing and laziness purposes).
 *
 * Prelude is split into quarters:  [zero][zero][sine][zero]
 * The sine frequency can be changed, as can the total length of the prelude.
 *
 * The drive signal is a logarithmically-swept sine (i.e. a chirp). The length
 * and start/end frequencies of this chirp can also be changed.  If padding is
 * added, this is put INSIDE the drive region, so longer padding results in
 * less time being spent in the desired frequency region.  If you use lots of
 * padding, increaase the drive length accordingly.
 *
 * Lead-out is silence.  It is unimportant, and can be removed once we allow
 * recording of a longer timespan than the played signal (or stop being lazy
 * and use two WAVEPARAMS structures... or pass length as a parameter... the
 * options are endless).
 */

/* State information for signal generator */
struct SIGNALGEN_STATE {
	/* Device configuration */
	int samplerate;

	/* Generator parameters */
	double prelude_len;
	double prelude_freq;
	double drive_padding;
	double chirp_low;
	double chirp_high;
	double drive_len;
	double drive_level;
	double duration;

	/* Timing data */
	struct {
		size_t position;
		size_t length;
		size_t prelude_offset;
		size_t prelude_samples;
		size_t drive_offset;
		size_t drive_samples;
		size_t drive_padding_samples;
		size_t sweep_offset;
		size_t sweep_samples;
	} timing;

	/* Synthesis data */
	struct {
		double prelude_omega;
		double chirp_phase;
		double chirp_omega_l;
		double chirp_omega_h;
		double chirp_logstart_omega;
		double chirp_logrange;
	} synth;

	/* Sweep rate (not used within the generator, for information only */
	double octavespersecond;

};

/* Initialise a signal generator */
struct SIGNALGEN_STATE* sgenInit(struct WAVEPARAMS* wp, double prelude_len,
		double prelude_freq, double drive_padding, double chirp_len,
		double chirp_low, double chirp_high, double drive_level);

/* Generate a signal */
void sgenGenerate(struct SIGNALGEN_STATE* state, double* buffer, size_t samples);

/*
 * Angular frequency at a certain index (no range checking is performed)
 */
double sgenChirpOmega(struct SIGNALGEN_STATE* state, size_t index);

/* Free a signal generator */
void sgenFree(struct SIGNALGEN_STATE* state);

#ifdef __cplusplus
}
#endif
