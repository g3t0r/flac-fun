#include "circle-buffer.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct CircleBuffer *newCircleBuffer(size_t capacity, size_t elemSize) {

  size_t sizeWithoutEntries = (size_t) &((struct CircleBuffer *) 0)->entries;
  size_t entriesSize = capacity * sizeof(struct CircleBufferEntry);

  struct CircleBuffer *buffer = malloc(sizeof *buffer);
  buffer->capacity = capacity;
  buffer->elemSize = elemSize;
  buffer->head = 0;
  buffer->tail = 0;
  buffer->currentSize = 0;
  buffer->entries = malloc(capacity * elemSize);
  return buffer;
}

void destroyCircleBuffer(struct CircleBuffer *buffer) {
  free(buffer->entries);
  free(buffer);
}

struct CircleBufferEntry *readEntryFromBuffer(struct CircleBuffer *buffer) {
  if (buffer->head == buffer->tail) {
    return NULL;
  }
  struct CircleBufferEntry *toReturn = buffer->entries + buffer->tail;
  buffer->tail = (buffer->tail + 1) % buffer->capacity;
  buffer->currentSize--;
  return toReturn;
}

struct CircleBufferEntry *writeDataToBuffer(struct CircleBuffer *buffer,
                                            const void *data, size_t size) {
  assert(buffer != NULL);
  assert(data != NULL);
  if ((buffer->head + 1) % buffer->capacity == buffer->tail) {
    return NULL;
  }

  (buffer->entries + buffer->head)->size = size;
  memcpy((buffer->entries + buffer->head)->data, data, size);

  buffer->head = (buffer->head + 1) % buffer->capacity;
  buffer->currentSize++;
  return (buffer->entries + buffer->head);
}
