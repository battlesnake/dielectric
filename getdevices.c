#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <MMSystem.h>
#include "wave.h"
#include "getdevices.h"

int GetInput(struct WAVEPARAMS* w) {
	WAVEINCAPS wic;
	UINT i, n;
	UINT devi;
	char s[256];
	n = waveInGetNumDevs();
	if (!n)
		return 0;
	puts("Select input device:");
	for (i = 0; i < n; i++) {
		printf("\t%d.\t", i+1);
		if (waveInGetDevCaps(i, &wic, sizeof(wic)) == MMSYSERR_NOERROR)
			puts(wic.szPname);
		else
			puts("<error>");
	}
	do {
		gets(s);
		devi = atoi(s);
	} while (devi < 1 || devi > n);
	puts("");
	w->dev_in = devi - 1;
	return 1;
}

int GetOutput(struct WAVEPARAMS* w) {
	WAVEOUTCAPS woc;
	UINT i, n;
	UINT devo;
	char s[256];
	n = waveOutGetNumDevs();
	if (!n)
		return 0;
	puts("Select output device:");
	for (i = 0; i < n; i++) {
		printf("\t%d.\t", i+1);
		if (waveOutGetDevCaps(i, &woc, sizeof(woc)) == MMSYSERR_NOERROR)
			puts(woc.szPname);
		else
			puts("<error>");
	}
	do {
		gets(s);
		devo = atoi(s);
	} while (devo < 1 || devo > n);
	puts("");
	w->dev_out = devo - 1;
	return 1;
}
