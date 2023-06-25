#include <FLAC/ordinals.h>
#include <stdint.h>
#define __STDC_FORMAT_MACROS

#include "FLAC/stream_decoder.h"
#include "FLAC/stream_encoder.h"
#include "inttypes.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

FLAC__StreamDecoderWriteStatus writeCallback(const FLAC__StreamDecoder *decoder,
                                             const FLAC__Frame *frame,
                                             const FLAC__int32 *const buffer[],
                                             void *client_data);

void metadataCallback(const FLAC__StreamDecoder *decoder,
                      const FLAC__StreamMetadata *metadata, void *client_data);

void errorCallback(const FLAC__StreamDecoder *decoder,
                   FLAC__StreamDecoderErrorStatus status, void *client_data) {
  printf("Hello from error callback");
}

void progressCallback(const FLAC__StreamEncoder *encoder,
                      FLAC__uint64 bytes_written, FLAC__uint64 samples_written,
                      uint32_t frames_written, uint32_t total_frames_estimate,
                      void *client_data);

struct ClientData {
  FLAC__StreamEncoder *encoder;
};

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

  struct ClientData * clientData = (struct ClientData *) malloc(sizeof(struct ClientData));

  FLAC__StreamDecoder *decoder = FLAC__stream_decoder_new();
  FLAC__StreamDecoderInitStatus status = FLAC__stream_decoder_init_FILE(
      decoder, readFd, writeCallback, metadataCallback, errorCallback, clientData);

  FLAC__bool ok = FLAC__stream_decoder_process_until_end_of_stream(decoder);
  if (!ok) {
    printf("Something is not OK\n");
  }
}

FLAC__StreamDecoderWriteStatus writeCallback(const FLAC__StreamDecoder *decoder,
                                             const FLAC__Frame *frame,
                                             const FLAC__int32 *const buffer[],
                                             void *client_data) {
}

void metadataCallback(const FLAC__StreamDecoder *decoder,
                      const FLAC__StreamMetadata *metadata, void *client_data) {
  FLAC__StreamEncoder * encoder = ((struct ClientData *) client_data)->encoder;
  FLAC__StreamMetadata *metadataCopy = malloc(sizeof(FLAC__StreamMetadata));
  memcpy(metadataCopy, metadata, sizeof(FLAC__StreamMetadata));
  FLAC__stream_encoder_set_metadata(encoder, &metadataCopy, 1);
}

void progressCallback(const FLAC__StreamEncoder *encoder,
                      FLAC__uint64 bytes_written, FLAC__uint64 samples_written,
                      uint32_t frames_written, uint32_t total_frames_estimate,
                      void *client_data) {
  float progress = (float)frames_written / (float) total_frames_estimate * 100.0;
  printf("Progress: %f.2\n", progress);
}
