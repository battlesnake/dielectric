#pragma once
#include <stdio.h>
#include "signalgen.h"
#include "history_buffer.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Record for window linked-list */
struct ANALYSER_WINDOW {
	size_t start;
	size_t length;
	size_t end;
	double mean;
	struct ANALYSER_WINDOW* next;
};

/* Individual results of analysis and decoding */
struct ANALYSER_RESULT {
	size_t idx;
	double freq;
	double value;
};

/* Aray of results of analysis and decoding */
struct ANALYSER_RESULTS {
	size_t count;
	size_t capacity;
	double time_error;
	double signal_to_noise;
	struct ANALYSER_RESULT* data;
};

/* Decode data */
struct ANALYSER_DECODE {
	/* FFT window width */
	size_t fft_window_width;

	/* Number of data points desired */
	size_t requested_datapoints;
};

/* Window list for resynchronisation data */
struct ANALYSER_RESYNC_WINDOW_LIST {
	struct ANALYSER_WINDOW* first;
	struct ANALYSER_WINDOW* last;
	size_t count;
};

/* Resynchronisation data */
struct ANALYSER_RESYNC {
	/* Resync minimum window width and spacing */
	size_t window_width;
	size_t window_spacing;
	
	/* Position of last window */
	size_t window_current_position;

	/*
	 * Window linked-list for bandpass-filtered data, (kept short by
	 * on-the-fly union-find algorithm)
	 */
	struct ANALYSER_RESYNC_WINDOW_LIST windows;

	/* Extreme values of window ms values */
	double window_max;

	/* Maximum allowed error in pulse width */
	double temporal_precision;

	/* Result of resync operation */
	struct ANALYSER_WINDOW pulse;
	size_t drive_start_position;
	size_t chirp_start_position;
};

/* State record for analysis */
struct ANALYSER_STATE {

	/* Signal generator used for original drive wave (used for timing info) */
	struct SIGNALGEN_STATE* sgen;

	/*
	 * Mode:
	 *   0 = resync (prelude pulse scan / fourier transform+union/find)
	 *   1 = decoding chirp (fourier transform)
	 */
	int mode;

	/*
	 * History: 
	 *   mode==0 : notchpass-filtered data
	 *   mode==1 : raw data
	 */
	struct HISTORY_BUFFER history;

	/* Resynchronisation (mode-0) */
	struct ANALYSER_RESYNC resync;
	int resync_init;

	/* Decoding (mode-1) */
	struct ANALYSER_DECODE decode;
	int decode_init;

	/* Output data */
	struct ANALYSER_RESULTS result;

	/* Finished? */
	int done;

	/* Error code and message */
	int errorcode;
	char error[200];

};

/* Initialise an analysis state record and analyse a complete signal */
struct ANALYSER_STATE* AnalyserInit(struct SIGNALGEN_STATE* sgen,
		double resync_window_size,
		double resync_window_spacing, double resync_temporal_precision,
		double decode_fft_window_width,
		size_t datapts);

/* Process part of a signal */
int AnalyserData(struct ANALYSER_STATE* state, double* data, size_t length);

/* Free an analysis state record */
void AnalyserFree(struct ANALYSER_STATE*);

/* Dump results to file */
int AnalyserDump(struct ANALYSER_STATE* state, FILE* file);

#ifdef __cplusplus
}
#endif
