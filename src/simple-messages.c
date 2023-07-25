#include "simple-messages.h"
#include "bytes.h"
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
uint32_t serializeMessageHeader(const struct MessageHeader *const header,
                                char *buffer) {
  int writtenBytes =
      writeIntegerToBuffer(buffer, &header->type, sizeof(header->size));

  writtenBytes += writeIntegerToBuffer(buffer + writtenBytes, &header->size,
                                       sizeof(header->size));

  return writtenBytes;
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
uint32_t deserializeMessageHeader(const char *const buffer,
                                  struct MessageHeader *header) {
  int readBytes =
      readIntegerFromBuffer(&header->type, buffer, sizeof(header->type));

  readBytes +=
      readIntegerFromBuffer(&header->size, buffer, sizeof(header->size));

  return readBytes;
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
uint32_t serializeDataMessage(const struct DataMessage *const message,
                              char *buffer) {
  int writtenBytes = writeIntegerToBuffer(
      buffer + writtenBytes, &message->dataSize, sizeof(message->dataSize));

  memcpy(buffer + writtenBytes, &message->data, message->dataSize);
  writtenBytes += message->dataSize;

  return writtenBytes;
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
uint32_t deserializeDataMessage(const char *const buffer,
                                struct DataMessage *message) {

  int readBytes = readIntegerFromBuffer(&message->dataSize, buffer,
                                        sizeof(message->dataSize));

  message->data = malloc(sizeof(char) * message->dataSize);
  memcpy(message->data, buffer + readBytes, message->dataSize);
  readBytes += message->dataSize;

  return readBytes;
}

/**
 * Function calculates number of bytes needed to serialize DataMessage,
 *          including memory pointed by DataMessage.data
 */
uint32_t dataMessageGetBytesLength(const struct DataMessage *const message) {
  return sizeof(message->dataSize) + message->dataSize * sizeof(char);
}
