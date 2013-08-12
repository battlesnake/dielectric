#include <stdlib.h>
#include <math.h>
#include "signed_t.h"
#include "mathutils.h"

double sqr(double v) {
	return v * v;
}

double rms(double* data, size_t length) {
	double sum = 0;
	signed_t i;
	for (i = 0; i < (signed_t) length; i++)
		sum += sqr(data[(size_t) i]);
	return sqrt(sum / length);
}

double rms_par(double* data, size_t length) {
	double sum = 0;
	signed_t i;
	#pragma omp parallel for reduction(+:sum)
	for (i = 0; i < (signed_t) length; i++)
		sum += sqr(data[(size_t) i]);
	return sqrt(sum / length);
}

double min_par(double* data, size_t length) {
	signed_t i;
	double result = data[0];
	#pragma omp parallel for
	for (i = 0; i < (signed_t) length; i++)	{
		double value = data[(size_t) i];
		if (value < result)
			#pragma omp critical
			{
				if (value < result)
					result = value;
			}
	}
	return result;
}

double max_par(double* data, size_t length) {
	signed_t i;
	double result = data[0];
	#pragma omp parallel for
	for (i = 0; i < (signed_t) length; i++)	{
		double value = data[(size_t) i];
		if (value > result)
			#pragma omp critical
			{
				if (value > result)
					result = value;
			}
	}
	return result;
}

void minmax_par(double* data, size_t length, double* min, double* max) {
	signed_t i;
	int hasresult = 0;
	*min = data[0];
	*max = data[0];
	#pragma omp parallel for
	for (i = 0; i < (signed_t) length; i++) {
		double value = data[(size_t) i];
		if (value < *min)
			#pragma omp critical
			{
				if (value < *min)
					*min = value;
			}
		if (value > *max)
			#pragma omp critical
			{
				if (value > *max)
					*max = value;
			}
	}
}

void minmaxsqr_par(double* data, size_t length, double* min, double* max) {
	signed_t i;
	int hasresult = 0;
	*min = sqr(data[0]);
	*max = sqr(data[0]);
	#pragma omp parallel for
	for (i = 0; i < (signed_t) length; i++) {
		double value = sqr(data[(size_t) i]);
		if (value < *min)
			#pragma omp critical
			{
				if (value < *min)
					*min = value;
			}
		if (value > *max)
			#pragma omp critical
			{
				if (value > *max)
					*max = value;
			}
	}
	*max = sqrt(*max);
	*min = sqrt(*min);
}

double middle_par(double* data, size_t length) {
	double min, max;
	minmax_par(data, length, &min, &max);
	return (min + max) / 2.0;
}

double middlesqr_par(double* data, size_t length) {
	double min, max;
	minmaxsqr_par(data, length, &min, &max);
	return (min + max) / 2.0;
}
