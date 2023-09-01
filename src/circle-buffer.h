#ifndef CIRCLE_BUFFER_H_
#define CIRCLE_BUFFER_H_

#include <stddef.h>

struct CircleBufferEntry {
  size_t size;
  char * data;
};

struct CircleBuffer {
  size_t capacity;
  size_t current_size;
  size_t elem_size;
  int head;
  int tail;
  struct CircleBufferEntry * entries;
};

struct CircleBuffer *circle_buffer_new(size_t capacity, size_t elemSize);
void circle_buffer_destroy(struct CircleBuffer *buffer);
struct CircleBufferEntry *circle_buffer_read(struct CircleBuffer *buffer);
struct CircleBufferEntry *circle_buffer_write(struct CircleBuffer *buffer,
                                            const void *data, size_t size);

#endif // CIRCLE_BUFFER_H_
