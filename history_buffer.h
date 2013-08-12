#pragma once
#include <stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif

/* History buffer state structure */
struct HISTORY_BUFFER {
	/* Buffer */
	double* re;
	double* im;
	/* Data type */
	int complex;
	/* Size of the buffer (elements) */
	size_t capacity;
	/* Amount of data in the buffer (elements) */
	size_t length;
	/* Element position of the end of the history buffer's data in source stream */
	size_t position;
	/* Element position of the start of the history buffer in the source stream */
	size_t start_offset;
	/* Amount of past data that must be preserved when more data is added */
	size_t overlap;
};

/* History buffer routines */
int History_Buffer_Init(struct HISTORY_BUFFER* buff);
void History_Buffer_Free(struct HISTORY_BUFFER* buff);
void History_Buffer_Clear(struct HISTORY_BUFFER* buff);
void History_Buffer_Clear_Set_Params(struct HISTORY_BUFFER* buff, size_t overlap, int complex);
int History_Buffer_Append(struct HISTORY_BUFFER* buff, double* re, double* im, size_t length);
int History_Buffer_Get_Data(struct HISTORY_BUFFER* buff, size_t stream_index, size_t length, double** re, double** im);

#ifdef __cplusplus
}
#endif
