#include <FLAC/ordinals.h>
#include <stdint.h>
#define __STDC_FORMAT_MACROS

#include "inttypes.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "FLAC/stream_decoder.h"

FLAC__StreamDecoderWriteStatus writeCallback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data) {
  //printf("Frame number: %u\n", frame->header.number.frame_number);
  //printf("Channel assignment: %u\n", frame->header.channel_assignment );
  return 0;
}

void metadataCallback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
  printf("Hello from metadata callback\n");
  printf("\tMetadataType: %u\n", metadata->type);
  printf("\tMin blocksize: %u\n", metadata->data.stream_info.min_blocksize);
  printf("\tMax blocksize: %u\n", metadata->data.stream_info.max_blocksize);
  printf("\tMin framesize: %u\n", metadata->data.stream_info.min_framesize);
  printf("\tMax framesize: %u\n", metadata->data.stream_info.max_framesize);
  printf("\tChannels: %u\n", metadata->data.stream_info.channels);
  printf("\tMetadataType: %u\n", metadata->type);
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

FLAC__bool ok =  FLAC__stream_decoder_process_until_end_of_stream(decoder);
if(!ok) {
  printf("Something is not OK\n");
}

  uint32_t blockSize = FLAC__stream_decoder_get_blocksize(decoder);
  uint32_t rate = FLAC__stream_decoder_get_sample_rate(decoder);
  uint32_t totalSamples = FLAC__stream_decoder_get_total_samples(decoder);
  printf("StreamDecoderStatus: %d\n", status);
  printf("SampleRate: %u\n", rate);
  printf("Total samples: %u\n", totalSamples);
  printf("Blocksize: %u\n", blockSize);
}
