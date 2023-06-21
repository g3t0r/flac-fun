#define __STDC_FORMAT_MACROS

#include "arpa/inet.h"
#include "bits.h"
#include "flac.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint64_t bigEndianToLittleEndian(uint64_t bigEndian) {
  uint64_t result = 0;
  uint64_t mask = 7;
  for(int i = 0; i < 8; i++) {
    result += bigEndian & mask;
    mask <<= 8;
  }
  return result;
}

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

  uint64_t buff64 = 0;
  r = readBytesBigEndian(&buff64, 8, fd);
  uint64_t sampleRateMask = (uint64_t)((1 << 20) - 1) << (64 - 20);
  uint64_t channelsMask = (uint64_t)7 << (64 - 20 - 3);
  uint64_t bitsPerSampleMask = (uint64_t)31 << (64 - 20 - 3 - 5);
  uint64_t totalSamplesInStreamMask = (uint64_t)((uint64_t)1<<36)-1;


  uint32_t sampleRate = (buff64 & sampleRateMask) >> (64-20);
  printf("Sample rate: %u\n", sampleRate);
  int channels = (buff64 & channelsMask) >> (64 - 20 - 3);
  printf("Channels: %u\n", channels);
  int bitsPerSample = (buff64 & bitsPerSampleMask) >> (64 - 20 - 3 - 5);
  printf("BitsPerSample: %u\n", bitsPerSample);
  uint64_t totalSamplesInStream = (buff64 & totalSamplesInStreamMask);
  printf("Total samples in stream: %"PRIu64"\n", totalSamplesInStream);
}
