#include "serializaiton.h"
#include "messages.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

struct {
  int fd;
  uint32_t buffer;
  int bitCounter;
} typedef BitWriter;

void serializeExampleMessage(int fd, struct ExampleMessage *message) {
  int total = sizeof(struct ExampleMessage);
}

uint32_t toNetworkByteOrder(uint32_t hostByteOrderInt) {
  uint32_t fullByteMask = 255;
  uint32_t converted = hostByteOrderInt << 24;
  converted += (hostByteOrderInt & (fullByteMask << 8)) << 8;
  converted += (hostByteOrderInt & (fullByteMask << 16)) >> 8;
  converted += (hostByteOrderInt & (fullByteMask << 24)) >> 24;

  return converted;
}

int writeBits(BitWriter *bw, const uint32_t *src, uint32_t numberOfBits) {
  uint32_t mask = (1 << numberOfBits) - 1;
  bw->bitCounter += numberOfBits;
  bw->buffer <<= numberOfBits;
  bw->buffer += (*src & mask);

  if (bw->bitCounter == 32) {
    int converted = toNetworkByteOrder(bw->buffer);
    int bytesWritten;
    while ((bytesWritten = write(bw->fd, &converted, sizeof(uint32_t))) != 0) {
      if(bytesWritten == -1) {
        return -1;
      }
    }
  }
  return 0;
}

void show(BitWriter *bitWriter) {
  int mask = 1 << 31;
  for (int i = 0; i < 32; i++) {
    if (i % 8 == 0) {
      printf(" ");
    }
    printf("%d", (bitWriter->buffer | mask) > 0 ? 1 : 0);
    mask >>= 1;
  }
  printf("\n");
}
