#include <Windows.h>
#include <MMSystem.h>
#include <stdlib.h>
#include <stdio.h>
#include "wave.h"
#include "transform.h"
#include "signalgen.h"
#include "graphing.h"

#define BUFFERCOUNT (4)

struct ECHOSTREAM_BUFFER;
struct ECHOSTREAM_BUFFER {
	double* data;
	HANDLE hasdata;
	struct ECHOSTREAM_BUFFER* arr;
	int* nexttoplay;
	int* nexttorec;
	int count;
	CRITICAL_SECTION* lock;
	int* loss_in;
	int* loss_out;
};

size_t EchoStreamOut_callback(struct WAVEPARAMS* wp, size_t position, double* buffer, size_t count, void* param, struct WAVE_DOUBLE_STREAM_EXTRA* extra) {
	struct ECHOSTREAM_BUFFER* meta = (struct ECHOSTREAM_BUFFER*) param;
	meta = &meta->arr[*meta->nexttoplay];
	WaitForSingleObject(meta->hasdata, INFINITE);
	memcpy(buffer, meta->data, count * sizeof(double));
	if (++(*meta->nexttoplay) == meta->count)
		*meta->nexttoplay = 0;
	EnterCriticalSection(meta->lock);
	*meta->loss_out = extra->misses;
	printf("\rLost packets: %d | %d", *meta->loss_in, *meta->loss_out);
	LeaveCriticalSection(meta->lock);
	return count;
}

void EchoStreamIn_callback(struct WAVEPARAMS* wp, size_t position, double* buffer, size_t count, void* param, struct WAVE_DOUBLE_STREAM_EXTRA* extra) {
	struct ECHOSTREAM_BUFFER* meta = (struct ECHOSTREAM_BUFFER*) param;
	meta = &meta->arr[*meta->nexttorec];
	memcpy(meta->data, buffer, count * sizeof(double));
	SetEvent(meta->hasdata);
	if (++(*meta->nexttorec) == meta->count)
		*meta->nexttorec = 0;
	extra->stop = 0;
	EnterCriticalSection(meta->lock);
	*meta->loss_in = extra->misses;
	printf("\rLost packets: %d | %d", *meta->loss_in, *meta->loss_out);
	LeaveCriticalSection(meta->lock);
	return;
}

int EchoStreamTest(struct WAVEPARAMS* w) {
	int wi, wo;
	struct ECHOSTREAM_BUFFER buffers[BUFFERCOUNT];
	int i;
	int nexttoplay = 0, nexttorec = 0;
	int loss_in = 0, loss_out = 0;
	CRITICAL_SECTION lock;
	for (i = 0; i < BUFFERCOUNT; i++) {
		buffers[i].data = (double*) malloc(sizeof(double) * MULTIBLOCK_BUFFERS_LENGTH);
		buffers[i].hasdata = CreateEvent(0, 0, 0, 0);
		buffers[i].arr = buffers;
		buffers[i].nexttoplay = &nexttoplay;
		buffers[i].nexttorec = &nexttorec;
		buffers[i].count = BUFFERCOUNT;
		buffers[i].lock = &lock;
		buffers[i].loss_in = &loss_in;
		buffers[i].loss_out = &loss_out;
	}
	w->length = 0;
	printf("\n");

	InitializeCriticalSection(&lock);

	#pragma omp parallel sections
	{
		#pragma omp section
		wo = WaveOut_Double_Stream(w, &EchoStreamOut_callback, buffers);
		#pragma omp section
		wi = WaveIn_Double_Stream(w, &EchoStreamIn_callback, buffers);
	}

	DeleteCriticalSection(&lock);

	for (i = 0; i < BUFFERCOUNT; i++) {
		free(buffers[i].data);
		CloseHandle(buffers[i].hasdata);
	}
	return wi && wo;
}
