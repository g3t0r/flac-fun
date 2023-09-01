#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "circle-buffer.h"
#include "logs.h"

struct CircleBuffer *circle_buffer_new(size_t capacity, size_t elem_size) {

  struct CircleBuffer *buffer = malloc(sizeof *buffer);
  buffer->capacity = capacity;
  buffer->elem_size = elem_size;
  buffer->head = 0;
  buffer->tail = 0;
  buffer->current_size = 0;
  buffer->entries = malloc(sizeof(struct CircleBufferEntry) * capacity);
  for (int i = 0; i < capacity; i++) {
    (buffer->entries+i)->data = malloc(elem_size);
  }
  return buffer;
}

void circle_buffer_destroy(struct CircleBuffer *buffer) {
  for (int i = 0; i < buffer->capacity; i++) {
    free((buffer->entries+i)->data);
  }
  free(buffer->entries);
  free(buffer);
}

struct CircleBufferEntry *circle_buffer_read(struct CircleBuffer *buffer) {
  if (buffer->head == buffer->tail) {
    print_debug("CB: returning null\n");
    return NULL;
  }
  struct CircleBufferEntry *output = buffer->entries + buffer->tail;
  buffer->tail = (buffer->tail + 1) % buffer->capacity;
  buffer->current_size--;
  return output;
}

struct CircleBufferEntry *circle_buffer_write(struct CircleBuffer *buffer,
                                            const void *data, size_t size) {
  assert(buffer != NULL);
  assert(data != NULL);
  if ((buffer->head + 1) % buffer->capacity == buffer->tail) {
    return NULL;
  }

  assert(size != 0);
  (buffer->entries + buffer->head)->size = size;
  memcpy((buffer->entries + buffer->head)->data, data, size);

  buffer->head = (buffer->head + 1) % buffer->capacity;
  buffer->current_size++;
  return (buffer->entries + buffer->head);
}
