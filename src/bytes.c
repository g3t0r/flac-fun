#include "bytes.h"
#include <stdint.h>

static inline uint32_t toggleEndianess32(uint32_t number);
static inline uint16_t toggleEndianess16(uint16_t number);

uint32_t toBigEndian32(uint32_t number) {
    return toggleEndianess32(number);
}

uint32_t toBigEndian16(uint16_t number) {
    return toggleEndianess16(number);
}

uint32_t toLittleEndian32(uint32_t number) {
    return toggleEndianess32(number);
}
uint32_t toLittleEndian16(uint16_t number) {
    return toggleEndianess16(number);
}

static inline uint32_t toggleEndianess32(uint32_t number) {
  uint32_t converted = 0;
  uint32_t mask = 0xFF;

  converted += (number & mask) << 24;
  converted += ((number >> 8) & mask) << 16;
  converted += ((number >> 16) & mask) << 8;
  converted += (number >> 24) & mask;

  return converted;
}

static inline uint16_t toggleEndianess16(uint16_t number) {
    uint16_t converted = (number << 8) + (number >> 8);
    return converted;
}
