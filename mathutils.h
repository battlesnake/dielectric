#include <math.h>
#ifdef __cplusplus
extern "C" {
#endif

#define PI	(3.1415926535897932384626433832795)
#define SQRT2	(1.4142135623730950488016887242097)
#define LN2	(0.69314718055994530941723212145818)

/* Square of parameter */
double sqr(double v);

/* Calculate RMS */
double rms(double* data, size_t length);

/* Calculate RMS (parallelised) */
double rms_par(double* data, size_t length);

/* Find minimum value (parallelised) */
double min_par(double* data, size_t length);

/* Find maximum value (parallelised) */
double max_par(double* data, size_t length);

/* Find minimum and maximum values (parallelised) */
void minmax_par(double* data, size_t length, double* min, double* max);

/* As above, but of squares of data - SEE IMPLEMENTATION */
void minmaxsqr_par(double* data, size_t length, double* min, double* max);

/* Find mean of minimum and maximum values (parallelised) */
double middle_par(double* data, size_t length);

/* As above, but of squares of data - SEE IMPLEMENTATION */
double middlesqr_par(double* data, size_t length);

#ifdef __cplusplus
}
#endif
