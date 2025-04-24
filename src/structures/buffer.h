#ifndef BUFFER_H
#define BUFFER_H

#include <stdlib.h>

typedef struct {
	int n;
	int cap;
	size_t elemSize;
	void *data;
} Buffer;

Buffer *createBuffers(int nBuffs, size_t elemSize);

Buffer *createBuffer(size_t elemSize);

void appendToBuffer(Buffer *buff, void *elem);

void freeBuffer(Buffer *buff);

void freeBuffers(Buffer *buffs, int nBuffs);

#endif
