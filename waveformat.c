#include <Windows.h>
#include <MMSystem.h>
#include "wave.h"

/* Populate a WAVEFORMATEX and WAVEHDR based on the values in a WAVEPARAMS */
void WaveGetFormat(struct WAVEPARAMS* wp, WAVEFORMATEX* wf, WAVEHDR* wh) {
	wf->cbSize = 0;
	wf->nChannels = wp->channels;
	wf->wBitsPerSample = wp->depth * 8;
	wf->nSamplesPerSec = wp->samplerate;
#ifndef PACKED_24BIT
	if (wp->depth == 3)
		wf->nBlockAlign = wp->channels * 4;
	else
		wf->nBlockAlign = wp->channels * wp->depth;
#else
	wf->nBlockAlign = wp->channels * wp->depth;
#endif
	wf->nAvgBytesPerSec = wf->nBlockAlign * wp->samplerate;
	wf->wFormatTag = WAVE_FORMAT_PCM;
	if (wh) {
		wh->dwBufferLength = (DWORD) (wf->nBlockAlign * wp->length);
		wh->dwFlags = 0;
		wh->dwLoops = 0;
		wh->dwUser = 0;
		wh->lpData = 0;
		wh->lpNext = 0;
		wh->dwBytesRecorded = 0;
		wh->reserved = 0;
	}
}
