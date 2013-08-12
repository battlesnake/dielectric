#pragma once
#ifdef __cplusplus
extern "C" {
#endif

/* Graph a data series */
#define GRAPHSERIES_USE_YMIN		0x0001
#define GRAPHSERIES_USE_YMAX		0x0002
#define GRAPHSERIES_HIDE_YAXIS		0x0010
#define GRAPHSERIES_HIDE_XAXIS		0x0020
#define GRAPHSERIES_HIDE_YLIMITS	0x0040
#define GRAPHSERIES_HIDE_EXTREMA	0x0100
#define GRAPHSERIES_HIDE_MEAN		0x0200
#define GRAPHSERIES_STAR_MIN		0x0400
#define GRAPHSERIES_STAR_MAX		0x0800
#define GRAPHSERIES_HIDE_CLIPPED	0x1000
#define GRAPHSERIES_FILL_RANGE		0x2000
void GraphSeries(double* data, size_t length, unsigned short width, unsigned short height, double ymin, double ymax, int flags);

#ifdef __cplusplus
}
#endif
