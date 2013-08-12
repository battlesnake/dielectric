#include <stdio.h>
#include <math.h>
#include "mathutils.h"
#include "signed_t.h"
#include "graphing.h"

#define GRAPHSERIES_NO_YAXIS	0x0010
#define GRAPHSERIES_NO_XAXIS	0x0020
#define GRAPHSERIES_NO_YLIMITS	0x0040
#define GRAPHSERIES_NO_EXTREMA	0x0100
#define GRAPHSERIES_NO_MEAN	0x0200

/* Graph a data series */
void GraphSeries(double* data, size_t length, unsigned short width, unsigned short height, double ymin, double ymax, int flags) {
	double sum, lmin, lmax;
	double max, min, range;
	int* pts;
	signed_t i;
	size_t count;
	int x, y;
	int show_y_axis = !(flags & GRAPHSERIES_HIDE_YAXIS);
	int show_x_axis = !(flags & GRAPHSERIES_HIDE_XAXIS);
	int show_y_limits = !(flags & GRAPHSERIES_HIDE_YLIMITS);
	int show_extrema = !(flags & GRAPHSERIES_HIDE_EXTREMA);
	int show_mean = !(flags & GRAPHSERIES_HIDE_MEAN);
	int fill_range = (flags & GRAPHSERIES_FILL_RANGE) != 0;
	int show_clipped = !(flags & GRAPHSERIES_HIDE_CLIPPED);
	int star_min = (flags & GRAPHSERIES_STAR_MIN) != 0;
	int star_max = (flags & GRAPHSERIES_STAR_MAX) != 0;
	if (!data || !length || !width || !height)
		return;
	if (flags & GRAPHSERIES_USE_YMAX)
		max = ymax;
	else
		max = max_par(data, length);
	if (flags & GRAPHSERIES_USE_YMIN)
		min = ymin;
	else
		min = min_par(data, length);
	range = max - min;
	/* Bin the data */
	sum = 0;
	count = 0;
	x = 0;
	pts = (int*) malloc(sizeof(*pts) * width * 3);
	for (i = 0; i < (signed_t) length; i++) {
		y = (int) floor((i * width * 1.0) / (length - 1));
		if (y > x) {
			/* Mean, min, max */
			pts[3*x+0] = (int) floor(0.5 + height * ((sum / count) - min) / range);
			pts[3*x+1] = (int) floor(0.5 + height * (lmin - min) / range);
			pts[3*x+2] = (int) floor(0.5 + height * (lmax - min) / range);
			x = y;
			count = 0;
			sum = 0;
		}
		if (!count || data[i] < lmin)
			lmin = data[i];
		if (!count || data[i] > lmax)
			lmax = data[i];
		sum += data[i];
		count++;
	}

	putchar('\n');

	/* y-axis max */
	if (show_y_limits)
		printf("%+.2g\n", max);

	/* high x-axis? */
	if (max < 0 && show_x_axis) {
		putchar(' ');
		putchar(' ');
		putchar(' ');
		putchar('+');
		for (x = 0; x < width; x++)
			putchar('-');
		putchar('\n');
	}

	/* Plot */
	for (y = height; y >= 0; y--) {
		int axis = 0;
		/* mid x-axis? */
		if (max >= 0 && min <= 0 && floor(0.5 + (-min * height) / range) == y)
			axis = 1;
		if (axis && show_x_axis) {
			putchar(' ');
			putchar('0');
			putchar(' ');
		}
		else {
			putchar(' ');
			putchar(' ');
			putchar(' ');
		}
		if (show_y_axis)
			putchar('|');
		for (x = 0; x < width; x++) {
			/* Mean */
			if (pts[3*x] == y && show_mean)
				putchar('*');
			/* Clipped */
			else if (show_clipped && (y == height && pts[3*x] > height || y == 0 && pts[3*x] < 0))
				putchar('?');
			/* Fill */
			else if (fill_range && y >= pts[3*x+1] && y <= pts[3*x+2])
				putchar('|');
			/* Extreme: star */
			else if (star_min && y == pts[3*x+1] || star_max && y == pts[3*x+2])
				putchar('*');
			/* Extreme: dot */
			else if (show_extrema && (y == pts[3*x+1] || y == pts[3*x+2]))
				putchar('.');
			/* Blank or mid x-axis */
			else
				if (axis && show_x_axis)
					putchar('-');
				else
					putchar(' ');
		}
		putchar('\n');
	}

	free(pts);

	/* low x-axis? */
	if (min > 0 && show_x_axis) {
		putchar(' ');
		putchar(' ');
		putchar(' ');
		putchar('+');
		for (x = 0; x < width; x++)
			putchar('-');
		putchar('\n');
	}

	/* y-axis min */
	if (show_y_limits)
		printf("%+.2g\n", min);

	putchar('\n');
	putchar('\n');
}
