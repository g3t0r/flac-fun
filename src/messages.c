#include "messages.h"
#include "bytes.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * Function converts MessageHeader into big endian bytes, and stored them
 *          into buffer
 *
 * @param message Pointer to MessageHeader.
 *
 * @param buffer Buffer in which bytes should be stored. Memory under this
 *               pointer has to be already allocated
 *
 * @return Number of bytes written to the buffer
 */
uint32_t messages_header_serialize(const struct MessageHeader *const header,
                                char *buffer) {
  int written_bytes =
      bytes_buffer_write_int(buffer, &header->type, sizeof(header->type));

  written_bytes += bytes_buffer_write_int(buffer + written_bytes, &header->size,
                                       sizeof(header->size));

  written_bytes += bytes_buffer_write_int(buffer + written_bytes, &header->seq,
                                       sizeof(header->seq));

  return written_bytes;
}

/**
 * Function convert bytes from buffer to MessageHeader.
 *
 * @param buffer Buffer of bytes stored in big endian
 *
 * @param message Pointer to message that bytes should be read into.
 *                Should be already allocated.
 *
 * @return Number of bytes read from the buffer
 */
uint32_t messages_header_deserialize(const char *const buffer,
                                  struct MessageHeader *header) {
  int read_bytes =
      bytes_buffer_read_int(&header->type, buffer, sizeof(header->type));

  read_bytes += bytes_buffer_read_int(&header->size, buffer + read_bytes,
                                     sizeof(header->size));

  read_bytes += bytes_buffer_read_int(&header->seq, buffer + read_bytes,
                                     sizeof(header->seq));

  return read_bytes;
}

/**
 * Function converts DataMessage into big endian bytes, and stored them into
 * buffer
 *
 * @param message Pointer to DataMessage
 *
 * @param buffer Buffer in which bytes should be stored. Memory under this
 *               pointer has to be already allocated
 *
 * @return Number of bytes written to the buffer
 */
uint32_t messages_data_msg_serialize(const struct DataMessage *const message,
                              char *buffer) {
  int written_bytes = bytes_buffer_write_int(buffer, &message->dataSize,
                                          sizeof(message->dataSize));

  memcpy(buffer + written_bytes, message->data, message->dataSize);
  written_bytes += message->dataSize;

  return written_bytes;
}

/**
 * Function convert bytes from buffer to DataMessage
 *
 * @param buffer Buffer of bytes stored in big endian
 * @param message Pointer to message that bytes should be read into.
 *                Should be already allocated.
 *
 * @return Number of bytes read from the buffer
 */
uint32_t messages_data_msg_deserialize(const char *const buffer,
                                struct DataMessage *message) {

  int read_bytes = bytes_buffer_read_int(&message->dataSize, buffer,
                                        sizeof(message->dataSize));

  message->data = malloc(sizeof(char) * message->dataSize);
  memcpy(message->data, buffer + read_bytes, message->dataSize);
  read_bytes += message->dataSize;

  return read_bytes;
}

/**
 * Function calculates number of bytes needed to serialize DataMessage,
 *          including memory pointed by DataMessage.data
 */
uint32_t messages_data_msg_get_length_bytes(const struct DataMessage *const message) {
  return sizeof(message->dataSize) + message->dataSize * sizeof(char);
}

uint16_t messages_feed_me_msg_serialize(const struct FeedMeMessage *message,
                                char *buffer) {
  return bytes_buffer_write_int(buffer, &message->dataSize,
                              sizeof(message->dataSize));
}

uint16_t messages_feed_me_msg_deserialize(const char *const buffer,
                                  struct FeedMeMessage *message) {
  return bytes_buffer_read_int(&message->dataSize, buffer,
                               sizeof(message->dataSize));
}
