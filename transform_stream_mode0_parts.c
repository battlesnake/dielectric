#include <stdlib.h>
#include <math.h>
#include "mathutils.h"
#include "transform.h"
#include "transform_internal.h"

int AnalyserUnionFind(struct ANALYSER_STATE* state) {
	struct ANALYSER_WINDOW* a;
	struct ANALYSER_WINDOW* b;
	double threshold;
	/* Get first window in list */
	a = state->resync.windows.first;
	/* Calculate threshold value */
	threshold = state->resync.window_max * THRESHOLD_SCALE_FACTOR;
	while (a && a->next) {
		struct ANALYSER_WINDOW* c;
		b = a->next;
		c = b->next;
		/* Find */
		if ((a->mean > threshold) == (b->mean > threshold) || c && (a->mean > threshold) && (c->mean > threshold)) {
			/* Union (note: overlapped areas are effectively double-weighted) */
			/* Union with next first window */
			a->mean = (a->mean * a->length + b->mean * b->length) / (a->length + b->length);
			a->end = b->end;
			a->length = a->end - a->start;
			/* Remove redundant window */
			a->next = c;
			free(b);
			/* Decrease window count*/
			state->resync.windows.count--;
			/* Set next window */
			a->next = c;
			/* Set last window if needed */
			if (!c)
				state->resync.windows.last = a;
		}
		/* Nothing to do, move to next window */
		else
			a = a->next;
	}

	return 1;
}

int ChirpAnalyseAppendFilteredWindow(struct ANALYSER_STATE* state, size_t centre, double mean) {
	struct ANALYSER_WINDOW* window;
	window = (struct ANALYSER_WINDOW*) malloc(sizeof(*window));
	if (!window) 
		return 0;

	/* Add window to list */
	window->start = state->resync.window_current_position - state->resync.window_width / 2;
	window->end = state->resync.window_current_position + state->resync.window_width / 2;
	window->length = window->end - window->start;
	window->mean = mean;
	window->next = 0;
	if (state->resync.windows.last) {
		state->resync.windows.last->next = window;
		state->resync.windows.last = window;
	}
	else {
		state->resync.windows.first = window;
		state->resync.windows.last = window;
	}

	/* Increase window count */
	state->resync.windows.count++;

	return 1;
}

int ChirpAnalyseFilteredWindow(struct ANALYSER_STATE* state, size_t centre) {
	double sr, si, mean;
	size_t i;
	double* re;
	double* im;
	size_t start, length;

	if (centre < state->resync.window_width / 2)
		return 1;

	/* Get pointer to start of window */
	if (centre < state->resync.window_width / 2)
		return 1;
	start = centre - state->resync.window_width / 2;
	length = state->resync.window_width;

	if (!History_Buffer_Get_Data(&state->history, start, length, &re, &im))
		return 1;

	/* Update window position in state record */
	state->resync.window_current_position = centre;

	/* Calculate mean-square over window */
	sr = 0;
	si = 0;
	for (i = 0; i < length; i++) {
		sr += re[i];
		si += im[i];
	}
	mean = sqrt(sqr(sr) + sqr(si)) / state->resync.window_width;
	if (mean > state->resync.window_max)
		state->resync.window_max = mean;

	/* Create new window record and add to linked list */
	if (!ChirpAnalyseAppendFilteredWindow(state, centre, mean))
		return 0;

	return 1;
}

/* Clear the window linked-list */
void ChirpAnalyseClearWindowList(struct ANALYSER_STATE* state) {
	struct ANALYSER_WINDOW* w = state->resync.windows.first;
	while (w) {
		struct ANALYSER_WINDOW* next = w->next;
		free(w);
		w = next;
	}
	state->resync.windows.first = 0;
	state->resync.windows.last = 0;
	state->resync.windows.count = 0;
}