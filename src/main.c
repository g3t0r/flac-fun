#define __STDC_FORMAT_MACROS

#include "inttypes.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "FLAC/stream_decoder.h"


int main(int argc, char **argv) {
  FILE *fd = fopen("audio/1.flac", "rb");
  if (fd == 0) {
    printf("Problem opening file");
    exit(1);
  }

  FLAC__StreamDecoder * decoder = FLAC__stream_decoder_new();
  FLAC__StreamDecoderInitStatus status = FLAC__stream_decoder_init_FILE(decoder, fd, NULL, NULL, NULL, NULL);
  printf("StreamDecoderStatus: %d\n", status);
}
