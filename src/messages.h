#ifndef SIMPLE_MESSAGES_H_
#define SIMPLE_MESSAGES_H_

#include <stdint.h>

#define MSG_HEADER_SIZE 5

struct MessageHeader {
  uint8_t type;
  uint32_t size;
};

struct DataMessage {
  uint32_t dataSize;
  char *data;
};

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
                                char *buffer);

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
                                  struct MessageHeader *header);

/**
 * Function converts DataMessage into big endian bytes, and stored them
 *          into buffer
 *
 * @param message Pointer to DataMessage.
 *
 * @param buffer Buffer in which bytes should be stored. Memory under this
 *               pointer has to be already allocated
 *
 * @return Number of bytes written to the buffer
 */
uint32_t serializeDataMessage(const struct DataMessage *const message,
                              char *buffer);

/**
 * Function convert bytes from buffer to DataMessage.
 *
 * @param buffer Buffer of bytes stored in big endian
 *
 * @param message Pointer to message that bytes should be read into.
 *                Should be already allocated.
 *
 * @return Number of bytes read from the buffer
 */
uint32_t deserializeDataMessage(const char *const buffer,
                                struct DataMessage *message);

/**
 * Function calculates number of bytes needed to serialize DataMessage,
 *          including memory pointed by DataMessage.data
 */
uint32_t dataMessageGetBytesLength(const struct DataMessage *const message);

#endif // SIMPLE_MESSAGES_H_
