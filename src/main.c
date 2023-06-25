#include <FLAC/ordinals.h>
#include <stdint.h>
#define __STDC_FORMAT_MACROS

#include "FLAC/stream_decoder.h"
#include "FLAC/stream_encoder.h"
#include "inttypes.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "decoder-callbacks.h"
#include "encoder-callbacks.h"

int main(int argc, char **argv) {
  FILE *readFd = fopen("audio/1.flac", "rb");
  if (readFd == 0) {
    printf("Problem opening read file");
    exit(1);
  }

  FILE *writeFD = fopen("audio/output.flac", "wb");
  if (writeFD == 0) {
    printf("Problem opening write file");
    exit(1);
  }

  FLAC__StreamEncoder *encoder = FLAC__stream_encoder_new();
  FLAC__StreamEncoderInitStatus initStatus =
      FLAC__stream_encoder_init_FILE(encoder, writeFD, progressCallback, NULL);

  if(initStatus != 0) {
    printf("Incorrect encoder status %u\n", initStatus);
    exit(1);
  }

  FLAC__StreamDecoder *decoder = FLAC__stream_decoder_new();
  FLAC__StreamDecoderInitStatus status = FLAC__stream_decoder_init_FILE(
      decoder, readFd, writeCallback, metadataCallback, errorCallback, NULL);

  FLAC__bool ok = FLAC__stream_decoder_process_until_end_of_metadata(decoder);
  if (!ok) {
    printf("Something wrong while reading metadata\n");
  }


}
