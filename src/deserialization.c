#include "deserialization.h"
#include <stdint.h>
#include <unistd.h>

struct {
  int fd;
  uint32_t buffer;
  int readBitsCounter;
} typedef BitReader;


uint32_t toHostByteOrderOrder(uint32_t networkByteOrderInt) {
  uint32_t fullByteMask = 255;
  uint32_t converted = networkByteOrderInt << 24;
  converted += (networkByteOrderInt & (fullByteMask << 8)) << 8;
  converted += (networkByteOrderInt & (fullByteMask << 16)) >> 8;
  converted += (networkByteOrderInt & (fullByteMask << 24)) >> 24;

  return converted;
}

readBitsUint32(BitReader * br, uint32_t * dst, uint32_t numberOfBits) {

  // but when to reverse byte order???
  if(br->readBitsCounter != -1) {
    read(br->fd, &br->buffer, sizeof(uint32_t));
  }

  int bitShift = 32 - numberOfBits - br->readBitsCounter;
  int bitsMask = ((1 << numberOfBits) - 1) << bitShift;

  *dst = (br->buffer & bitsMask) >> bitShift;
  br->readBitsCounter += numberOfBits;
}
