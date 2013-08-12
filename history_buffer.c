#include <memory.h>
#include "history_buffer.h"

int History_Buffer_Init(struct HISTORY_BUFFER* buff) {
	if (!buff)
		return 0;
	memset(buff, 0, sizeof(struct HISTORY_BUFFER));
	return 1;
}

void History_Buffer_Free(struct HISTORY_BUFFER* buff) {
	free(buff->re);
	free(buff->im);
	buff->re = 0;
	buff->im = 0;
}
