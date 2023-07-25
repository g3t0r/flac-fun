#ifndef BYTES_H_
#define BYTES_H_

#include <stdint.h>

uint32_t toBigEndian32(uint32_t number);
uint32_t toBigEndian16(uint16_t number);

uint32_t toLittleEndian32(uint32_t number);
uint32_t toLittleEndian16(uint16_t number);



#endif // BYTES_H_
