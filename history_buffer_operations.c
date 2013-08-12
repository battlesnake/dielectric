#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include "history_buffer.h"

int History_Buffer_Expand(struct HISTORY_BUFFER* buff, size_t newcapacity) {
	double* larger = 0;
	/* Do we need to expand? */
	if (newcapacity < buff->capacity)
		return 1;
	/* Rellocate real buffer */
	larger = (double*) realloc(buff->re, sizeof(double) * newcapacity);
	if (!larger)
		return 0;
	buff->re = larger;
	/* Rellocate imaginary buffer */
	if (buff->complex) {
		larger = (double*) realloc(buff->im, sizeof(double) * newcapacity);
		if (!larger)
			return 0;
		buff->im = larger;
	}
	/* Store new capacity */
	buff->capacity = newcapacity;
	return 1;
}

int History_Buffer_Shift_Left(struct HISTORY_BUFFER* buff, size_t amount) {
	size_t tomove;
	/* Fail if data will be lost */
	if (amount > buff->length) 
		return 0;
	/* Amount of data to move */
	tomove = buff->length - amount;
	/* Perform the move */
	if (tomove) {
		memmove(buff->re, buff->re + amount, sizeof(double) * tomove);
		if (buff->complex)
			memmove(buff->im, buff->im + amount, sizeof(double) * tomove);
	}
	/* Increase the buffer start offset and decrease the buffer length */
	buff->start_offset += amount;
	buff->length -= amount;
	return 1;
}

int History_Buffer_Make_Space(struct HISTORY_BUFFER* buff, size_t length) {
	/* Do we already have space? */
	if (length + buff->length <= buff->capacity) 
		return 1;
	/* Make space by shifting to the left */
	return History_Buffer_Shift_Left(buff, length + buff->length - buff->capacity);
}

int History_Buffer_Append(struct HISTORY_BUFFER* buff, double* re, double* im, size_t length) {
	/* Expand buffer if needed */
	if (!History_Buffer_Expand(buff, length + buff->overlap))
		return 0;
	/* Make space if needed */
	if (!History_Buffer_Make_Space(buff, length))
		return 0;
	/* Add the data to the buffer */
	memcpy(buff->re + buff->length, re, sizeof(double) * length);
	if (buff->complex)
		memcpy(buff->im + buff->length, im, sizeof(double) * length);
	/* Increase the buffer length and stream position */
	buff->length += length;
	buff->position += length;
	return 1;
}

void History_Buffer_Clear(struct HISTORY_BUFFER* buff) {
	/* Increase the buffer start offset */
	buff->start_offset += buff->length;
	/* Clear and free the buffer */
	buff->length = 0;
	buff->capacity = 0;
	free(buff->re);
	free(buff->im);
	buff->re = 0;
	buff->im = 0;
}

void History_Buffer_Clear_Set_Params(struct HISTORY_BUFFER* buff, size_t overlap, int complex) {
	/* Clear the buffer */
	History_Buffer_Clear(buff);
	/* Set buffer parameters */
	buff->overlap = overlap;
	buff->complex = complex;
}

int History_Buffer_Get_Data(struct HISTORY_BUFFER* buff, size_t stream_index, size_t length, double** re, double** im) {
	if ((im == 0) != (buff->complex == 0))
		return 0;
	if (stream_index < buff->start_offset)
		return 0;
	if (stream_index + length > buff->position)
		return 0;
	*re = buff->re + (stream_index - buff->start_offset);
	if (im)
		*im = buff->im + (stream_index - buff->start_offset);
	return 1;
}
