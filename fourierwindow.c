#include <stdlib.h>
#include <math.h>
#include "mathutils.h"
#include "fourierwindow.h"

int FourierWindow(double freq, double *input, double* outputr, double* outputi, size_t position, size_t length, size_t samplerate) {
	size_t i;
	double domega_dsample = 2 * PI * freq / samplerate;
	for (i = 0; i < length; i++) {
		outputr[i] = input[i] * cos(domega_dsample * (i + position));
		outputi[i] = input[i] * sin(domega_dsample * (i + position));
	}
	return 1;
}
