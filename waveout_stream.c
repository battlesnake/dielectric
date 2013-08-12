#include <stdlib.h>
#include <Windows.h>
#include <MMSystem.h>
#include "wave.h"

#ifdef _WIN32

struct WAVEOUT_DOUBLE_STREAM {
	/* Number of buffers in queue */
	volatile unsigned int buffersinuse;
};

void __stdcall WAVEOUTCALLBACK_DOUBLE_STREAM(HWAVEIN h, UINT msg, DWORD_PTR dwInstance,
	DWORD_PTR p1, DWORD_PTR p2) {
	struct WAVEOUT_DOUBLE_STREAM* meta = (struct WAVEOUT_DOUBLE_STREAM*) dwInstance;
	if (msg == WOM_DONE) {
		/* Uses an event to request data from the main thread */
		WAVEHDR* wh = (WAVEHDR*) p1;

		/* Set block state to "unused" */
		wh->dwUser = 0;
		InterlockedDecrement(&meta->buffersinuse);
	}
}
    
int WaveOut_Double_Stream(struct WAVEPARAMS* wp, WAVEOUT_DOUBLE_STREAM_CALLBACK callback, void* param) {
	WAVEFORMATEX wf;
	HWAVEOUT hw = 0;
	MMRESULT mmr;
	struct WAVEOUT_DOUBLE_STREAM meta;
	WAVEHDR wh[MULTIBLOCK_BUFFERS_COUNT];
	size_t i;
	int result = 0;
	size_t position, length;
	double* buffer = 0;
	int buffer_index;
	size_t bufferlen;
	int nomoredata;
	int dataneeded;
	WAVEHDR* hdr;
	struct WAVE_DOUBLE_STREAM_EXTRA extra;
	
	/* Initialise */
	memset(&meta, 0, sizeof(meta));
	for (i = 0; i < MULTIBLOCK_BUFFERS_COUNT; i++)
		memset(wh+i, 0, sizeof(wh[i]));
	
	/* Check for callback */
	if (!callback)
		goto exit;
	
	/* Get corresponding OS wave format params */
	WaveGetFormat(wp, &wf, wh);
	
	/* Open device */
	if ((mmr = waveOutOpen(&hw, wp->dev_out, &wf, (DWORD_PTR) &WAVEOUTCALLBACK_DOUBLE_STREAM, (DWORD_PTR) &meta, CALLBACK_FUNCTION | WAVE_FORMAT_DIRECT)) != MMSYSERR_NOERROR)
		goto exit;
	
	/* Create buffers and initialise wave headers */
	for (i = 0; i < MULTIBLOCK_BUFFERS_COUNT; i++) {
		hdr = wh + i;
		hdr->dwFlags = (i > 0) ? 0 : WHDR_BEGINLOOP;
		hdr->dwBufferLength = wf.nBlockAlign * MULTIBLOCK_BUFFERS_LENGTH;
		hdr->lpData = (LPSTR) malloc(hdr->dwBufferLength);
		if (!hdr->lpData)
			goto exit;
		hdr->dwUser = 0;
	}

	/* Stream */
	position = 0;
	length = wp->length;
	
	/* Allocate temporary buffer */
	buffer = (double*) malloc(sizeof(*buffer) * MULTIBLOCK_BUFFERS_LENGTH);

	/* Keep sending blocks as they're requested */
	nomoredata = 0;
	dataneeded = 1;
	buffer_index = 0;
	memset(&extra, 0, sizeof(extra));
	extra.bufferstotal = MULTIBLOCK_BUFFERS_COUNT;
	do {
		/* If more data is needed, prepare next buffer */
		if (dataneeded && !nomoredata) {
			/* Calculate how many samples are remaining */
			size_t samplesneeded;
			if (length == 0 || position + MULTIBLOCK_BUFFERS_LENGTH <= length)
				samplesneeded = MULTIBLOCK_BUFFERS_LENGTH;
			else
				samplesneeded = length - position;
			/* Get data */
			extra.buffersinuse = meta.buffersinuse;
			bufferlen = callback(wp, position, buffer, samplesneeded, param, &extra);
			if (samplesneeded == 0)
				extra.stop = 1;
			/* Check for erroneous return value */
			if (bufferlen > samplesneeded)
				goto exit;
			/* Not stopping? Set the has-data flag. */
			if (!extra.stop)
				dataneeded = 0;
		}
		else
			callback(wp, position, buffer, 0, param, &extra);

		/* Check for failure */
		if (extra.fail)
			goto exit;

		/* Stopping? Set no-more-data-wanted flag. */
		if (extra.stop)
			nomoredata = 1;

		/* Get block */
		hdr = wh + buffer_index;

		/* Is a block free and do we have data left to stream? */
		if (hdr->dwUser == 0 && bufferlen > 0 && (position < length || length == 0)) {
			/* Unprepare header if needed */
			if (hdr->dwFlags & WHDR_PREPARED)
				if ((mmr = waveOutUnprepareHeader(hw, hdr, sizeof(WAVEHDR))) != MMSYSERR_NOERROR)
					goto exit;
			/* Update header */
			hdr->dwFlags = 0;
			hdr->dwBufferLength = (DWORD) (wf.nBlockAlign * bufferlen); /* typecast: assume buffer size is less than 4GB */
			/* Fill buffer */
			for (i = 0; i < bufferlen; i++)
				w_write(wp, (void*) hdr->lpData, i, buffer[i]);
			position += bufferlen;
			/* Is this the last buffer? */
			if (position == length)
				hdr->dwFlags |= WHDR_ENDLOOP;
			/* Prepare header */
			if ((mmr = waveOutPrepareHeader(hw, hdr, sizeof(WAVEHDR))) != MMSYSERR_NOERROR)
				goto exit;
			/* Mark block as in use */
			hdr->dwUser = 1;
			/* Dispatch buffer */
			if ((mmr = waveOutWrite(hw, hdr, sizeof(WAVEHDR))) != MMSYSERR_NOERROR)
				goto exit;
			if (meta.buffersinuse == 0 && position > bufferlen)
				extra.misses++;
			InterlockedIncrement(&meta.buffersinuse);
			/* Need more data */
			dataneeded = 1;
			bufferlen = 0;
		}
		else
			/* If idle, yield to other threads */
			Sleep(0);

		/* Advance to next block */
		if (++buffer_index == MULTIBLOCK_BUFFERS_COUNT)
			buffer_index = 0;

	} while (meta.buffersinuse > 0 || !(extra.stop || extra.fail));

	/* Clean up */
	result = 1;
exit:
	WaveLastError = mmr;
	free(buffer);
	if (hw) {
		waveOutReset(hw);
		for (i = 0; i < MULTIBLOCK_BUFFERS_COUNT; i++)
			if (wh[i].dwFlags & WHDR_PREPARED)
				waveOutUnprepareHeader(hw, &wh[i], sizeof(WAVEHDR));
		waveOutClose(hw);
	}
	for (i = 0; i < MULTIBLOCK_BUFFERS_COUNT; i++)
		free(wh[i].lpData);
	return result;
}

#endif
