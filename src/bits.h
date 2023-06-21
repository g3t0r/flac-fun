#ifndef BITS_H_
#define BITS_H_

#include "stdio.h"
#include "inttypes.h"
#include <stdint.h>

int readBytesBigEndian(void *ptr, int n, FILE *fd);
uint8_t calculateShift(uint8_t numberOfBits, uint8_t numberOfBitsInBuffer, size_t bufferSize);
uint8_t getNumberOrBits(int integer);

#endif // BITS_H_
