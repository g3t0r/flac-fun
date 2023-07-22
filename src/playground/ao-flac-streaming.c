#include <FLAC/format.h>
#include <FLAC/ordinals.h>
#include <FLAC/stream_decoder.h>
#include <ao/ao.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

FILE *fd;

sem_t semaphore;
int metadata = 1;

ao_device *device;
ao_sample_format format;
int default_driver;
char *buffer;
int buf_size;
int sample;

void metadataCb() { metadata = 0; }

FLAC__StreamDecoderReadStatus readCb(const FLAC__StreamDecoder *decoder,
                                     FLAC__byte buffer[], size_t *bytes,
                                     void *clientData) {
  if (metadata == 0) {
    int semValue;
    sem_getvalue(&semaphore, &semValue);
    printf("Semaphore value: %d\n", semValue);
  }
  printf("decoder tries to read %lu bytes\n", *bytes);
  size_t readBytes = fread(buffer, sizeof(char), *bytes, fd);
  *bytes = readBytes;
  printf("provided %lu bytes\n", readBytes);

  return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

FLAC__StreamDecoderWriteStatus writeCb(const FLAC__StreamDecoder *decoder,
                                       const FLAC__Frame *frame,
                                       const int *const *buffer,
                                       void *clientData) {

  printf("decoder tries to write %u bytes\n", frame->header.blocksize);
  format.byte_format = AO_FMT_BIG;
  format.channels = frame->header.channels;
  format.bits = frame->header.bits_per_sample;

  uint32_t blocksize = frame->header.blocksize;
  uint32_t bytesPerSample = format.bits / 8;
  uint32_t numberOfChannels = format.channels;

  char * tmpBuffer = malloc(blocksize);

  for(int i = 0; i < blocksize; i+= bytesPerSample) {
    int channel = (i / bytesPerSample) % numberOfChannels;
    memcpy(tmpBuffer + i, const void *, unsigned long)
  }






  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void errorCb(const FLAC__StreamDecoder *decoder,
             FLAC__StreamDecoderErrorStatus status, void *clientData) {}

int main() {
  ao_initialize();
  default_driver = ao_default_driver_id();
  memset(&format, 0, sizeof(format));

  sem_init(&semaphore, 0, 5);

  fd = fopen("../../audio/1.flac", "rb");

  FLAC__StreamDecoder *decoder = FLAC__stream_decoder_new();
  FLAC__stream_decoder_init_stream(decoder, readCb, NULL, NULL, NULL, NULL,
                                   writeCb, metadataCb, errorCb, NULL);

  FLAC__stream_decoder_process_until_end_of_metadata(decoder);
  printf("After metadata\n");
  FLAC__stream_decoder_process_until_end_of_stream(decoder);

  return 0;
}
