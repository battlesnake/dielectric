#pragma once
#include <Windows.h>
#include <MMSystem.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Wave interface: separate input/output devices and a common format to use with both */
struct WAVEPARAMS {
	UINT dev_in, dev_out;	/* device		      */
	DWORD samplerate;	/* sample rate		      */
	WORD channels;		/* channels		      */
	WORD depth;		/* depth (in bytes, not bits) */
	size_t length;		/* blocks		      */
};

/* Play/record a waveform buffer */
int WaveIn(struct WAVEPARAMS*, void*);
int WaveOut(struct WAVEPARAMS*, void*);

/* See if a wave format is supported by the given device */
int WaveInTest(UINT device, DWORD samplerate, WORD depth, WORD channels);
int WaveOutTest(UINT device, DWORD samplerate, WORD depth, WORD channels);

/* Number of bytes required for wave buffer */
size_t wavebuffersize(struct WAVEPARAMS *w);

/* Read/write from a wave buffer */
double w_read(struct WAVEPARAMS* w, void* buffer, size_t idx);
void w_write(struct WAVEPARAMS* w, void* buffer, size_t idx, double value);

/* Populate a WAVEFORMATEX and WAVEHDR based on the values in a WAVEPARAMS */
void WaveGetFormat(struct WAVEPARAMS* wp, WAVEFORMATEX* wf, WAVEHDR* wh);

/* Double-precision wave i/o */
#define MULTIBLOCK_BUFFERS_COUNT 8
#define MULTIBLOCK_BUFFERS_LENGTH 8000
int WaveOut_Double(struct WAVEPARAMS* wp, double* buffer);
int WaveIn_Double(struct WAVEPARAMS* wp, double* buffer);

/* Streaming double-precision wave i/o */
struct WAVE_DOUBLE_STREAM_EXTRA {
	int buffersinuse;
	int bufferstotal;
	int misses;
	int stop;
	int fail;
};
/* Return number of samples to play */
typedef size_t (*WAVEOUT_DOUBLE_STREAM_CALLBACK)(struct WAVEPARAMS* wp, size_t position, double* buffer, size_t count, void* param, struct WAVE_DOUBLE_STREAM_EXTRA* extra);
typedef void (*WAVEIN_DOUBLE_STREAM_CALLBACK)(struct WAVEPARAMS* wp, size_t position, double* buffer, size_t count, void* param, struct WAVE_DOUBLE_STREAM_EXTRA* extra);
/* Streaming audio i/o */
int WaveOut_Double_Stream(struct WAVEPARAMS* wp, WAVEOUT_DOUBLE_STREAM_CALLBACK callback, void* param);
int WaveIn_Double_Stream(struct WAVEPARAMS* wp, WAVEIN_DOUBLE_STREAM_CALLBACK callback, void* param);

/* error code */
extern MMRESULT WaveLastError;

#ifdef __cplusplus
}
#endif
