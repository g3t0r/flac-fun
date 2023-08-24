#ifndef CIRCLE_BUFFER_H_
#define CIRCLE_BUFFER_H_

#include <stddef.h>

struct CircleBufferEntry {
  size_t size;
  char data[];
};

struct CircleBuffer {
  size_t capacity;
  size_t currentSize;
  size_t elemSize;
  int head;
  int tail;
  struct CircleBufferEntry * entries;
};

struct CircleBuffer *newCircleBuffer(size_t capacity, size_t elemSize);
void destroyCircleBuffer(struct CircleBuffer *buffer);
struct CircleBufferEntry *readEntryFromBuffer(struct CircleBuffer *buffer);
struct CircleBufferEntry *writeDataToBuffer(struct CircleBuffer *buffer,
                                            const void *data, size_t size);

#endif // CIRCLE_BUFFER_H_
