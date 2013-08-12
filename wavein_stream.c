#include <stdlib.h>
#include <Windows.h>
#include <MMSystem.h>
#include "wave.h"

#ifdef _WIN32

struct WAVEIN_DOUBLE_STREAM_MSG;
struct WAVEIN_DOUBLE_STREAM_MSG {
	WAVEHDR* wh;
	int requeued;
	size_t position;
	size_t samples;
	struct WAVEIN_DOUBLE_STREAM_MSG* prev;
	struct WAVEIN_DOUBLE_STREAM_MSG* next;
};

struct WAVEIN_DOUBLE_STREAM {
	volatile unsigned int buffersinuse;
	/* Events */
	HANDLE recording_complete;
	HANDLE hasmessage;
	int done;
	/* Output stream */
	size_t length;
	size_t position;
	/* Wave stuff */
	WAVEFORMATEX* wf;
	/* Message list lock */
	CRITICAL_SECTION lock;
	/* Message queue (linked list) */
	struct WAVEIN_DOUBLE_STREAM_MSG* messages;
	struct WAVEIN_DOUBLE_STREAM_MSG* messagelast;
};

void __stdcall WAVEINCALLBACK_DOUBLE_STREAM(HWAVEIN h, UINT msg, DWORD_PTR dwInstance, DWORD_PTR p1, DWORD_PTR p2) {
	struct WAVEIN_DOUBLE_STREAM* meta = (struct WAVEIN_DOUBLE_STREAM*) dwInstance;
	if (msg == WIM_DATA && WaitForSingleObject(meta->recording_complete, 0) != WAIT_OBJECT_0) {

		/* Uses a message queue to dispatch copy of buffer back to the main thread for processing */
		WAVEHDR* wh = (WAVEHDR*) p1;
		struct WAVEIN_DOUBLE_STREAM_MSG* msg;
		size_t samples;
		int recording_complete = (WaitForSingleObject(meta->recording_complete, 0) == WAIT_OBJECT_0);

		InterlockedDecrement(&meta->buffersinuse);

		/* Range check */
		samples = wh->dwBytesRecorded / (meta->wf->wBitsPerSample / 8);
		if (meta->position + samples > meta->length && meta->length > 0) {
			samples = meta->length - meta->position;
			recording_complete = 1;
		}

		/* Create message */
		msg = (struct WAVEIN_DOUBLE_STREAM_MSG*) malloc(sizeof(*msg));
		msg->samples = wh->dwBytesRecorded / (meta->wf->wBitsPerSample / 8);
		msg->wh = wh;
		msg->requeued = 0;
		msg->prev = meta->messagelast;
		msg->next = 0;
		msg->position = meta->position;

		/* Advance stream pointer */
		meta->position += samples;

		/* Dispatch message */
		EnterCriticalSection(&meta->lock);
		if (WaitForSingleObject(meta->recording_complete, 0) != WAIT_OBJECT_0) {
			if (meta->messagelast)
				meta->messagelast->next = msg;
			else
				meta->messages = msg;
			meta->messagelast = msg;
		}
		else
			free(msg);
		LeaveCriticalSection(&meta->lock);

		/* Trigger message handler */
		SetEvent(meta->hasmessage);

		/*
		 * Dispatch "recording_complete" message if we're about to fill the output buffer completely.
		 * This event MUST be set AFTER the "hasmessage" event, or the last block will
		 * not get processed.
		 */
		if (recording_complete)
			SetEvent(meta->recording_complete);
	}
}

int WaveIn_Double_Stream_GetMessage(struct WAVEIN_DOUBLE_STREAM_MSG* message, struct WAVEIN_DOUBLE_STREAM* meta) {
	struct WAVEIN_DOUBLE_STREAM_MSG* msg;
	/* Lock */
	EnterCriticalSection(&meta->lock);
	/* Get first message and remove it from the queue */
	msg = meta->messages;
	if (!msg) {
		LeaveCriticalSection(&meta->lock);
		return 0;
	}
	meta->messages = msg->next;
	if (msg->next)
		msg->next->prev = 0;
	if (msg == meta->messagelast)
		meta->messagelast = 0;
	/* If there are no more messages, reset the "has message" event and possibly set the "recording_complete" event */
	if (!meta->messages) {
		ResetEvent(meta->hasmessage);
		if (meta->done)
			SetEvent(meta->recording_complete);
	}
	/* Unlock */
	LeaveCriticalSection(&meta->lock);
	/* Copy message to result, free original message and return */
	*message = *msg;
	free(msg);
	return 1;
}

void WaveIn_Double_Stream_ClearMessages(struct WAVEIN_DOUBLE_STREAM* meta) {
	struct WAVEIN_DOUBLE_STREAM_MSG msg;
	while (WaveIn_Double_Stream_GetMessage(&msg, meta))
		;
}

int WaveIn_Double_Stream(struct WAVEPARAMS* wp, WAVEIN_DOUBLE_STREAM_CALLBACK callback, void* param) {
	WAVEFORMATEX wf;
	HWAVEIN hw = 0;
	MMRESULT mmr;
	struct WAVEIN_DOUBLE_STREAM meta;
	WAVEHDR wh[MULTIBLOCK_BUFFERS_COUNT];
	size_t i;
	int result = 0;
	HANDLE events[2];
	double* buffer = 0;
	void* buffercopy = 0;
	void* devbuffers = 0;
	size_t devbufferlen;
	DWORD waitresult;
	struct WAVE_DOUBLE_STREAM_EXTRA extra;
	
	/* Initialise */
	memset(&meta, 0, sizeof(meta));
	for (i = 0; i < MULTIBLOCK_BUFFERS_COUNT; i++)
		memset(wh+i, 0, sizeof(wh[i]));
	
	/* Check for buffer */
	if (!callback)
		goto exit;
	
	/* Get corresponding OS wave format params */
	WaveGetFormat(wp, &wf, wh);
	
	/* Open device */
	if ((mmr = waveInOpen(&hw, wp->dev_in, &wf, (DWORD_PTR) &WAVEINCALLBACK_DOUBLE_STREAM, (DWORD_PTR) &meta, CALLBACK_FUNCTION | WAVE_FORMAT_DIRECT)) != MMSYSERR_NOERROR)
		goto exit;
	
	/* Create device buffers and initialise wave headers */
	devbufferlen = MULTIBLOCK_BUFFERS_LENGTH * wf.nBlockAlign;
	devbuffers = malloc(devbufferlen * MULTIBLOCK_BUFFERS_COUNT);
	if (!devbuffers)
		goto exit;
	for (i = 0; i < MULTIBLOCK_BUFFERS_COUNT; i++) {
		wh[i].dwBufferLength = (DWORD) devbufferlen;			/* Assume buffer size < 4GB */
		wh[i].lpData = (LPSTR) ((char*) devbuffers + i*devbufferlen);
	}
	
	/* Prepare headers */
	for (i = 0; i < MULTIBLOCK_BUFFERS_COUNT; i++)
		if ((mmr = waveInPrepareHeader(hw, wh + i, sizeof(WAVEHDR))) != MMSYSERR_NOERROR)
			goto exit;
	
	/* Initialise metadata */
	memset(&meta, 0, sizeof(meta));
	meta.recording_complete = CreateEvent(0, 1, 0, 0);
	meta.wf = &wf;
	meta.length = wp->length;
	meta.hasmessage = CreateEvent(0, 1, 0, 0);
	InitializeCriticalSection(&meta.lock);
	
	/* Allocate temporary buffers for raw data and converted data */
	buffer = (double*) malloc(sizeof(*buffer) * MULTIBLOCK_BUFFERS_LENGTH);
	buffercopy = malloc(devbufferlen);
	if (!buffercopy)
		goto exit;

	/* Add buffers to queue */
	for (i = 0; i < MULTIBLOCK_BUFFERS_COUNT; i++)
		if ((mmr = waveInAddBuffer(hw, wh + i, sizeof(WAVEHDR))) != MMSYSERR_NOERROR)
			goto exit;
	
	/* Start recording */
	if ((mmr = waveInStart(hw)) != MMSYSERR_NOERROR)
		goto exit;

	/* Process all messages, break loop when recording is recording_complete */
	memset(&extra, 0, sizeof(extra));
	extra.bufferstotal = MULTIBLOCK_BUFFERS_COUNT;
	events[0] = meta.hasmessage;
	events[1] = meta.recording_complete;
	while ((waitresult = WaitForMultipleObjects(2, events, 0, INFINITE)) == WAIT_OBJECT_0)  {
		struct WAVEIN_DOUBLE_STREAM_MSG msg;
		
		if (!WaveIn_Double_Stream_GetMessage(&msg, &meta))
			goto exit;

		if (!meta.done) {
			/* Copy data from device buffer */
			memcpy(buffercopy, msg.wh->lpData, msg.wh->dwBytesRecorded);

			/* Requeue device buffer */
			if (meta.buffersinuse == 0)
				extra.misses++;
			if ((mmr = waveInAddBuffer(hw, msg.wh, sizeof(WAVEHDR))) != MMSYSERR_NOERROR)
				goto exit;
			InterlockedIncrement(&meta.buffersinuse);
	
			/* Process buffer */
			for (i = 0; i < msg.samples; i++)
				buffer[i] = w_read(wp, buffercopy, i);

			/* Invoke callback */
			extra.buffersinuse = meta.buffersinuse;
			callback(wp, msg.position, buffer, msg.samples, param, &extra);
			if (extra.fail)
				goto exit;
			else if (extra.stop)
				meta.done = 1;
		}
	}
	/* Check for wait error */
	if (waitresult != WAIT_OBJECT_0 + 1)
		goto exit;

	/* Clean up */
	result = 1;
exit:
	WaveLastError = mmr;
	if (hw) {
		waveInReset(hw);
		for (i = 0; i < MULTIBLOCK_BUFFERS_COUNT; i++)
			if (wh[i].dwFlags & WHDR_PREPARED)
				waveInUnprepareHeader(hw, &wh[i], sizeof(WAVEHDR));
		WaveIn_Double_Stream_ClearMessages(&meta);
		waveInClose(hw);
	}
	free(buffer);
	free(buffercopy);
	free(devbuffers);
	if (meta.recording_complete)
		CloseHandle(meta.recording_complete);
	if (meta.hasmessage) {
		CloseHandle(meta.hasmessage);
		DeleteCriticalSection(&meta.lock);
	}
	return result;
}

#endif
