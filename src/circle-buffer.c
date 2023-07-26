#include "circle-buffer.h"
#include <stdlib.h>

struct CircleBuffer *newCircleBuffer(size_t capacity) {
  struct CircleBuffer *buffer = malloc(sizeof(*buffer));
  buffer->capacity = capacity;
  buffer->head = 0;
  buffer->tail = 0;
  buffer->entries = malloc(sizeof(*buffer->entries) * capacity);
  return buffer;
}

void destroyCircleBuffer(struct CircleBuffer *buffer) {
  struct CircleBufferEntry *entry;
  while ((entry = readEntryFromBuffer(buffer)) != NULL) {
    free(entry->data);
    entry->data = NULL;
  }

  free(buffer->entries);
  free(buffer);
}

struct CircleBufferEntry *readEntryFromBuffer(struct CircleBuffer *buffer) {
  if (buffer->head == buffer->tail) {
    return NULL;
  }
  struct CircleBufferEntry *toReturn = buffer->entries + buffer->tail;
  buffer->tail = (buffer->tail + 1) % buffer->capacity;
  return toReturn;
}

struct CircleBufferEntry *writeDataToBuffer(struct CircleBuffer *buffer,
                                            void *data, size_t size) {
  if ((buffer->head + 1) % buffer->capacity == buffer->tail) {
    return NULL;
  }

  (buffer->entries + buffer->head)->size = size;
  (buffer->entries + buffer->head)->data = data;

  buffer->head = (buffer->head + 1) % buffer->capacity;
  return (buffer->entries + buffer->head);
}
