#ifndef SIMPLE_MESSAGES_H_
#define SIMPLE_MESSAGES_H_

#include <stddef.h>
#include <stdint.h>

#define MSG_HEADER_SIZE 5
#define MSG_FEED_ME_SIZE 2

enum MessageType { HEARTBEAT, DATA, FEED_ME };

struct MessageHeader {
  uint32_t seq;
  uint16_t size;
  uint8_t type;
};

struct DataMessage {
  uint32_t dataSize;
  char *data;
};

struct FeedMeMessage {
  uint16_t dataSize;
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
uint32_t messages_header_serialize(const struct MessageHeader *const header,
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
uint32_t messages_header_deserialize(const char *const buffer,
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
uint32_t messages_data_msg_serialize(const struct DataMessage *const message,
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
uint32_t messages_data_msg_deserialize(const char *const buffer,
                                struct DataMessage *message);

/**
 * Function calculates number of bytes needed to serialize DataMessage,
 *          including memory pointed by DataMessage.data
 */
uint32_t messages_data_msg_get_length_bytes(const struct DataMessage *const message);

uint16_t messages_feed_me_msg_serialize(const struct FeedMeMessage *message,
                                char *buffer);

uint16_t messages_feed_me_msg_deserialize(const char *const buffer,
                                  struct FeedMeMessage *message);

#endif // SIMPLE_MESSAGES_H_
