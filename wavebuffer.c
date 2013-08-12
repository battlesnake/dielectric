#include <Windows.h>
#include "wave.h"

#ifdef _WIN32

/* Number of bytes required for wave buffer */
size_t wavebuffersize(struct WAVEPARAMS *w) {
#ifndef PACKED_24BIT
	if (w->depth == 3)
		return w->length * 4;
#endif
	return w->length * w->depth;
}

/* Decode a floating-point value from a device-format wave buffer */
double w_read(struct WAVEPARAMS* w, void* buffer, size_t idx) {
	if (w->depth == 1)
		return *((signed char*) buffer + idx) * 1.0 / 0x7f;
	if (w->depth == 2)
		return *((signed short*) buffer + idx) * 1.0 / 0x7fff;
#ifdef PACKED_24BIT
	if (w->depth == 3) {
		signed int v = (*(signed int*) ((char*) buffer + 3*idx));
		int neg = v & 0x800000;
		v &= 0x7fffff;
		if (neg)
			v = v - 0x800000;
		return v * 1.0 / 0x7fffff;
	}
#else
	if (w->depth == 3)
		return *((signed int*) buffer + idx) * 1.0 / 0x7fffff;
#endif
	else
		return 0;
}

/* Encode a floating-point value to a device-format wave buffer */
void w_write(struct WAVEPARAMS* w, void* buffer, size_t idx, double value) {
	if (w->depth == 1)
		*((signed char*) buffer + idx) = (signed char) (signed int) (0x7f * value);
	if (w->depth == 2)
		*((signed short*) buffer + idx) = (signed short) (signed int) (0x7fff * value);
#ifdef PACKED_24BIT
	if (w->depth == 3)
		*(signed int*) ((char*) buffer + 3*idx) = (signed int) (0x7fffff * value) & 0x7fffff;
#else
	if (w->depth == 3)
		*((signed int*) buffer + idx) = (signed int) (0x7fffff * value);
#endif
}

#endif