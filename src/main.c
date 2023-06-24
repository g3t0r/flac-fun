#define __STDC_FORMAT_MACROS

#include "inttypes.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "FLAC/stream_decoder.h"

FLAC__StreamDecoderWriteStatus writeCallback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data) {
  printf("Hello from write callback :)\n");
  return 0;
}

void metadataCallback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
  printf("Hello from metadata callback");
}

void errorCallback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data) {
  printf("Hello from error callback");
}

int main(int argc, char **argv) {
  FILE *fd = fopen("audio/1.flac", "rb");
  if (fd == 0) {
    printf("Problem opening file");
    exit(1);
  }

  FLAC__StreamDecoder * decoder = FLAC__stream_decoder_new();
  FLAC__StreamDecoderInitStatus status = FLAC__stream_decoder_init_FILE(decoder, fd, writeCallback, metadataCallback, errorCallback, NULL);
  printf("StreamDecoderStatus: %d\n", status);
}
