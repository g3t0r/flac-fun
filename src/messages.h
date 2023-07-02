#ifndef FFUN_MESSAGES_H_
#define FFUN_MESSAGES_H_

#include <stdint.h>

enum { EXAMPLE_STRUCT = 0 } typedef MessageType;

struct ExampleMessage {
  uint8_t messageType;
  int size;
  uint8_t oneByteData;
  uint16_t twoByteData;
  uint32_t fourByteData;
  char rawArrayOfBytes[10];
};

#endif // FFUN_MESSAGES_H_
