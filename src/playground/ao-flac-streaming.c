#include <FLAC/format.h>
#include <FLAC/ordinals.h>
#include <FLAC/stream_decoder.h>
#include <ao/ao.h>
#include <math.h>
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
  // printf("decoder tries to read %lu bytes\n", *bytes);
  size_t readBytes = fread(buffer, sizeof(char), *bytes, fd);
  *bytes = readBytes;
  // printf("provided %lu bytes\n", readBytes);

  return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

FLAC__StreamDecoderWriteStatus writeCb(const FLAC__StreamDecoder *decoder,
                                       const FLAC__Frame *frame,
                                       const FLAC__int32 *const *buffer,
                                       void *clientData) {

  uint32_t blocksize = frame->header.blocksize; // number of samples
  uint32_t bytesPerSample = 3;
  uint32_t numberOfChannels = 2;
  uint32_t rate = 44100;

  uint32_t bufferSize = blocksize * bytesPerSample * numberOfChannels;
  printf("Buffer size: %u\n", bufferSize);

  uint32_t mask = 0xFF; // full byte

  char *tmpBuffer = calloc(bufferSize, sizeof(char));

  for (int i = 0; i < blocksize; i++) {

    int leftChannel = i * bytesPerSample * numberOfChannels;
    int rightChannel = i * bytesPerSample * numberOfChannels + bytesPerSample;

    tmpBuffer[leftChannel] = (int8_t)(buffer[0][i] >> 16) & mask;
    tmpBuffer[rightChannel] = (int8_t)(buffer[1][i] >> 16) & mask;
    tmpBuffer[leftChannel + 1] = (int8_t)(buffer[0][i] >> 8) & mask;
    tmpBuffer[rightChannel + 1] = (int8_t)(buffer[1][i] >> 8) & mask;
    tmpBuffer[leftChannel + 2] = (int8_t)buffer[0][i] & mask;
    tmpBuffer[rightChannel + 2] = (int8_t)buffer[1][i] & mask;
  }

  // buf_size = format.bits / 8 * format.channels * format.rate;
  // char *tmpBuffer = calloc(buf_size, sizeof(char));

  // buf_size = format.bits / 8 * format.channels * format.rate;
  // buffer = calloc(buf_size, sizeof(char));

  // int freq = 432;
  // for (int i = 0; i < format.rate; i++) {
  //   sample =
  //       (int)(0.75 * 32768.0 * sin(2 * M_PI * freq * ((float)i /
  //       format.rate)));

  //   /* Put the same stuff in left and right channel */
  //   tmpBuffer[4 * i] = tmpBuffer[4 * i + 2] = sample & 0xff;
  //   tmpBuffer[4 * i + 1] = tmpBuffer[4 * i + 3] = (sample >> 8) & 0xff;
  // }

  // ao_play(device, tmpBuffer, buf_size);

  int playR = ao_play(device, tmpBuffer, bufferSize);
  free(tmpBuffer);
  printf("Errno: %d\n", errno);
  printf(strerror(errno));

  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void errorCb(const FLAC__StreamDecoder *decoder,
             FLAC__StreamDecoderErrorStatus status, void *clientData) {}

int main() {
  ao_initialize();
  default_driver = ao_default_driver_id();

  printf("default driver: %d\n", default_driver);
  memset(&format, 0, sizeof(format));
  format.byte_format = AO_FMT_BIG;
  format.channels = 2;
  format.bits = 24;
  format.rate = 44100;

  ao_option option = {"verbose"};
  device = ao_open_live(default_driver, &format, &option);
  if (device == NULL) {
    printf("Device == null\n");
  }

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
