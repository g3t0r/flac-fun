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
uint32_t serializeMessageHeader(const struct MessageHeader *const header,
                                char *buffer) {
  int writtenBytes =
      writeIntegerToBuffer(buffer, &header->type, sizeof(header->type));

  writtenBytes += writeIntegerToBuffer(buffer + writtenBytes, &header->size,
                                       sizeof(header->size));

  writtenBytes += writeIntegerToBuffer(buffer + writtenBytes, &header->seq,
                                       sizeof(header->seq));

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

  readBytes += readIntegerFromBuffer(&header->size, buffer + readBytes,
                                     sizeof(header->size));
  readBytes += readIntegerFromBuffer(&header->seq, buffer + readBytes,
                                     sizeof(header->seq));

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
  int writtenBytes = writeIntegerToBuffer(buffer, &message->dataSize,
                                          sizeof(message->dataSize));

  memcpy(buffer + writtenBytes, message->data, message->dataSize);
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

uint16_t serializeFeedMeMessage(const struct FeedMeMessage *message,
                                char *buffer) {
  return writeIntegerToBuffer(buffer, &message->dataSize,
                              sizeof(message->dataSize));
}

uint16_t deserializeFeedMeMessage(const char *const buffer,
                                  struct FeedMeMessage *message) {
  return readIntegerFromBuffer(&message->dataSize, buffer,
                               sizeof(message->dataSize));
}
