#ifndef BYTES_H_
#define BYTES_H_

#include <stddef.h>
#include <stdint.h>

struct ByteArray {
  size_t size;
  char buffer[];
};

/**
 * Function reads integer of certain SIZE from SRC to DST
 * Reason for this method is to abstract away endianess conversion
 *
 * @param dst Pointer to memory value should be stored in.
 *
 * @param src Pointer to memory value should be read from.
 *
 * @return size Bytes written to buffer.
 */
uint32_t bytes_buffer_read_int(void * dst, const void * const src, uint8_t size);


/**
 * Function writes integer of certain SIZE from SRC to DST
 * Reason for this method is to abstract away endianess conversion
 *
 * @param dst Pointer to memory value should be stored in.
 *
 * @param src Pointer to memory value should be read from.
 *
 * @param size size of src integer in bytes
 *
 * @return size Bytes written into buffer
 *
 *
 */
uint32_t bytes_buffer_write_int(void * dst, const void * const src, uint8_t size);

uint32_t bytes_convert_to_big_endian_32(uint32_t number);
uint32_t bytes_convert_to_big_endian_16(uint16_t number);

uint32_t bytes_convert_to_little_endian_32(uint32_t number);
uint32_t bytes_convert_to_little_endian_16(uint16_t number);



#endif // BYTES_H_
