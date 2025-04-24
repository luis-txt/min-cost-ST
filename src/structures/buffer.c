#include <string.h>

#include "buffer.h"

static void setBuffer(Buffer *buff, size_t elemSize) {
	buff->n = 0;
	buff->cap = 1024;
	buff->elemSize = elemSize;
	buff->data = calloc(buff->cap, elemSize);
}

Buffer *createBuffers(int nBuffs, size_t elemSize) {
	Buffer *buffs = calloc(nBuffs, sizeof(Buffer));
	for (int i = 0; i < nBuffs; i++) {
		setBuffer(&buffs[i], elemSize);
	}
	return buffs;
}

Buffer *createBuffer(size_t elemSize) {
	Buffer *buff = calloc(1, sizeof(Buffer));
	setBuffer(buff, elemSize);
	return buff;
}

void appendToBuffer(Buffer *buff, void *elem) {
	if (buff->n >= buff->cap) {
		buff->cap = (buff->cap == 0) ? 1024 : buff->cap * 2;
		buff->data = realloc(buff->data, buff->cap * buff->elemSize);
	}
	memcpy((char*)buff->data + (buff->n * buff->elemSize), elem, buff->elemSize);
	buff->n++;
}

void freeBuffer(Buffer *buff) {
	free(buff->data);
	free(buff);
}

void freeBuffers(Buffer *buffs, int nBuffs) {
	for (int i = 0; i < nBuffs; i++) {
		free(buffs[i].data);
	}
	free(buffs);
}
