#include <Windows.h>
#include <MMSystem.h>
#include "wave.h"
#include "test.h"

#ifdef _WIN32

int WaveInTest(UINT device, DWORD samplerate, WORD depth, WORD channels) {
	struct WAVEPARAMS wp;
	WAVEFORMATEX wf;
	MMRESULT mmr;
	HWAVEIN hw = 0;
	wp.dev_in = device;
	wp.channels = channels;
	wp.depth = depth;
	wp.length = 0;
	wp.samplerate = samplerate;
	WaveGetFormat(&wp, &wf, 0);
	if ((mmr = waveInOpen(&hw, device, &wf, 0, 0, WAVE_FORMAT_DIRECT | WAVE_FORMAT_QUERY | CALLBACK_NULL)) != MMSYSERR_NOERROR)
		return 0;
	if (hw)
		waveInClose(hw);
	return 1;
}

int WaveOutTest(UINT device, DWORD samplerate, WORD depth, WORD channels) {
	struct WAVEPARAMS wp;
	WAVEFORMATEX wf;
	MMRESULT mmr;
	HWAVEOUT hw = 0;
	wp.dev_out = device;
	wp.channels = channels;
	wp.depth = depth;
	wp.length = 0;
	wp.samplerate = samplerate;
	WaveGetFormat(&wp, &wf, 0);
	/* Don't use WAVE_FORMAT_QUERY, the result isn't always accurate */
	if ((mmr = waveOutOpen(&hw, wp.dev_out, &wf, 0, 0, WAVE_FORMAT_DIRECT | WAVE_FORMAT_QUERY | CALLBACK_NULL)) != MMSYSERR_NOERROR)
		return 0;
	if (hw)
		waveOutClose(hw);
	return 1;
}

#endif
