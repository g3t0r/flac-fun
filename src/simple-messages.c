#include "simple-messages.h"

/**
 * Function converts DataMessage into big endian bytes, and stored them into
 * buffer
 *
 * @param message Pointer to DataMessage
 * @param buffer Buffer in which bytes should be stored. Memory under this
 *               pointer has to be already allocated
 *
 * @return Number of bytes written to the buffer
 */
uint32_t serializeDataMessage(const struct DataMessage *const message,
                              char *buffer);

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
                                struct DataMessage *message);

/**
 * Function calculates number of bytes needed to serialize DataMessage,
 *          including memory pointed by DataMessage.data
 */
uint32_t dataMessageGetBytesLength(const struct DataMessage *const message);


static int readIntegerFromBuffer(int fd, void *integer, size_t size) {
  int readBytes = 0;
  if (size == sizeof(uint8_t)) {


  } else if (size == sizeof(uint16_t)) {

    uint16_t networkByteOrder;
    readLoop(fd, &networkByteOrder, size);
    *(uint16_t *)integer = ntohs(networkByteOrder);

  } else {
    assert(sizeof(uint32_t) == size);
    uint32_t netwotkByteOrder;
    readLoop(fd, &netwotkByteOrder, size);
    *(uint32_t *)integer = ntohl(netwotkByteOrder);
    int breakpointPlaceholder = 0;
  }
  return size;
}
