#include "arpa/inet.h"
#include "flac.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int readBytesBigEndian(unsigned int *ptr, int n, FILE *fd);

int main(int argc, char **argv) {
  FILE *fd = fopen("audio/1.flac", "rb");
  if (fd == 0) {
    printf("Problem opening file");
    exit(1);
  }
  char marker[4];
  fread(&marker, sizeof(marker), 1, fd);

  MetadataBlockHeader header;

  uint8_t bufer;

  fread(&bufer, sizeof(bufer), 1, fd);

  int lastBitMask = 1 << 7;
  int lastBit = (bufer & lastBitMask) >> 7;
  int blockTypeMask = lastBitMask - 1;
  int blockType = (bufer & blockTypeMask);
  printf("block type: %d\n", blockType);
  int r = 0;
  uint32_t length = 0;
  r = readBytesBigEndian((unsigned int *)&length, 3, fd);
  printf("R = %d\n", r);
  printf("Length: %d\n", length);

  uint16_t minimumBlockSize = 0;
  r = readBytesBigEndian((unsigned int *)&minimumBlockSize, 2, fd);
  printf("R = %d\n", r);
  printf("Minimum block size: %d\n", minimumBlockSize);

  uint16_t maximumBlockSize = 0;
  r = readBytesBigEndian((unsigned int *)&maximumBlockSize, 2, fd);
  printf("R = %d\n", r);
  printf("Minimum block size: %d\n", maximumBlockSize);

  uint32_t minimumFrameSize = 0;
  uint32_t maximumFrameSize = 0;
  r = readBytesBigEndian((unsigned int *)&minimumFrameSize, 3, fd);
  printf("R = %d\n", r);
  printf("MinimumFrameSize: %d\n", minimumFrameSize);
  r = readBytesBigEndian((unsigned int *)&maximumFrameSize, 3, fd);
  printf("R = %d\n", r);
  printf("MaximumFrameSize: %d\n", maximumFrameSize);
}

int readBytesBigEndian(unsigned int *ptr, int n, FILE *fd) {
  uint32_t total = 0;
  int r = 0;
  for (int i = 0; i < n; i++) {
    if (i != 0) {
      total <<= 8;
    }
    int tmp = 0;
    r += fread(&tmp, sizeof(uint8_t), 1, fd);
    total += tmp;
  }
  *ptr = total;
  return r;
}
