#include "bits.h"
#include <stdint.h>


int readBytesBigEndian(void *ptr, int n, FILE *fd) {
  int r = 0;
  for (int i = 0; i < n; i++) {
    if (i != 0) {
       *(uint64_t *) ptr <<= 8;
    }
    uint8_t tmp = 0;
    r += fread(&tmp, sizeof(uint8_t), 1, fd);
    *(uint16_t *)ptr += tmp;
  }
  return r;
}

uint8_t calculateShift(uint8_t numberOfBits, uint8_t numberOfBitsInBuffer, size_t bufferSize) {
    return bufferSize * 8 - numberOfBitsInBuffer - numberOfBits;
}

uint8_t getNumberOrBits(int integer) {
    uint8_t number = 0;
    while(integer != 0) {
       number++;
       integer >>= 1;
    }
    return number;
}
