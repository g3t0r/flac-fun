#include "bytes.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

static inline uint32_t toggleEndianess32(uint32_t number);
static inline uint16_t toggleEndianess16(uint16_t number);

/**
 * Function reads integer of certain SIZE from SRC to DST
 *          Reason for this method is to abstract away
 *          endianess conversion
 *
 * @param dst Pointer to integer that value should be stored in.
 *
 * @param src Pointer to memory value should be read from.
 *
 * @param size size of src integer in bytes
 *
 * @return size Bytes read from buffer
 *
 */
uint32_t readIntegerFromBuffer(void *dst, const void *const src, uint8_t size) {

  if (size == sizeof(uint8_t)) {
    memcpy(dst, src, size);
    return size;
  }

  if (size == sizeof(uint16_t)) {
    uint16_t tmp;
    memcpy(&tmp, src, size);
    *(uint16_t *)dst = toLittleEndian16(tmp);
    return size;
  }

  assert(size == sizeof(uint32_t));
  uint32_t tmp;
  memcpy(&tmp, src, size);
  *(uint32_t *)dst = toLittleEndian32(tmp);
  return size;
}

/**
 * Function writes integer of certain SIZE from SRC to DST
 *          Reason for this method is to abstract away
 *          endianess conversion.
 *
 * @param dst Pointer to memory value should be stored in.
 *
 * @param src Pointer to memory value should be read from.
 *
 * @param size size of src integer in bytes
 *
 * @return size Bytes written into buffer
 *
 */

uint32_t writeIntegerToBuffer(void *dst, const void *const src, uint8_t size) {
  if (size == sizeof(uint8_t)) {
    *(char *)dst = *(char *)src;
    return size;
  }

  if (size == sizeof(uint16_t)) {
    uint16_t tmp = toBigEndian16(*(uint16_t *)src);
    memcpy(dst, &tmp, size);
    return size;
  }

  assert(size == sizeof(uint32_t));
  uint32_t tmp = toBigEndian32(*(uint32_t *)src);
  memcpy(dst, &tmp, size);
  return size;
}

uint32_t toBigEndian32(uint32_t number) { return toggleEndianess32(number); }
uint32_t toBigEndian16(uint16_t number) { return toggleEndianess16(number); }

uint32_t toLittleEndian32(uint32_t number) { return toggleEndianess32(number); }
uint32_t toLittleEndian16(uint16_t number) { return toggleEndianess16(number); }

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
