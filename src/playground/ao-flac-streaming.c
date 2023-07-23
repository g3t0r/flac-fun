/*
** how to compile:
** gcc ao-flac-streaming.c circle-buffer.c -lFLAC -lao -ldl -lm -lpthread
 */
#include "circle-buffer.h"
#include <FLAC/format.h>
#include <FLAC/ordinals.h>
#include <FLAC/stream_decoder.h>
#include <ao/ao.h>
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define CIRCLE_BUFFER_SIZE 10

FLAC__StreamDecoderWriteStatus writeCb(const FLAC__StreamDecoder *decoder,
                                       const FLAC__Frame *frame,
                                       const FLAC__int32 *const *buffer,
                                       void *clientData);

FLAC__StreamDecoderReadStatus readCb(const FLAC__StreamDecoder *decoder,
                                     FLAC__byte buffer[], size_t *bytes,
                                     void *clientData);

void errorCb(const FLAC__StreamDecoder *decoder,
             FLAC__StreamDecoderErrorStatus status, void *clientData) {}

void metadataCb() { printf("ME TA DA TA\n"); }

void *feedBuffer();

FILE *fd;

struct CircleBuffer *circleBuffer;

sem_t writeSemaphore;
sem_t readSemaphore;
int metadata = 1;

ao_device *device;
ao_sample_format format;
int default_driver;
char *buffer;
int buf_size;
int sample;

int main() {
  ao_initialize();

  default_driver = ao_default_driver_id();
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

  sem_init(&writeSemaphore, 0, CIRCLE_BUFFER_SIZE - 1);
  sem_init(&readSemaphore, 0, 0);

  fd = fopen("../../audio/1.flac", "rb");
  circleBuffer = newCircleBuffer(CIRCLE_BUFFER_SIZE);
  pthread_t thread;
  pthread_create(&thread, NULL, feedBuffer, NULL);

  FLAC__StreamDecoder *decoder = FLAC__stream_decoder_new();
  FLAC__stream_decoder_init_stream(decoder, readCb, NULL, NULL, NULL, NULL,
                                   writeCb, metadataCb, errorCb, NULL);

  FLAC__stream_decoder_process_until_end_of_metadata(decoder);
  printf("After metadata\n");
  FLAC__stream_decoder_process_until_end_of_stream(decoder);

  return 0;
}

FLAC__StreamDecoderReadStatus readCb(const FLAC__StreamDecoder *decoder,
                                     FLAC__byte buffer[], size_t *bytes,
                                     void *clientData) {
  sem_wait(&readSemaphore);
  struct CircleBufferEntry *entry = readEntryFromBuffer(circleBuffer);
  assert(entry != NULL);
  memcpy(buffer, entry->data, entry->size);
  *bytes = entry->size;
  sem_post(&writeSemaphore);
  free(entry->data);
  entry->data = NULL;
  entry->size = 0;
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

  int playR = ao_play(device, tmpBuffer, bufferSize);
  free(tmpBuffer);
  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

size_t iteration = 0;

void *feedBuffer() {
  char buffer[8192];
  size_t readBytes;
  while ((readBytes = fread(buffer, sizeof(char), 8192, fd)) != 0) {
    printf("Feeding buffer %lu bytes, iteration: %lu\n", readBytes, iteration);
    int readSemValue, writeSemValue;
    sem_getvalue(&readSemaphore, &readSemValue);
    sem_getvalue(&writeSemaphore, &writeSemValue);
    printf("Read sem: %d, write value %d\n", readSemValue, writeSemValue);
    printf("Read value: %d, write value %d\n", readSemValue, writeSemValue);
    char *data = malloc(sizeof(char) * readBytes);
    sem_wait(&writeSemaphore);
    memcpy(data, buffer, readBytes);
    struct CircleBufferEntry *entry =
        writeDataToBuffer(circleBuffer, data, readBytes);
    assert(entry != NULL);
    // network latency simulation
    if (iteration % 50 > 15) {
      usleep(100 * 1000);
    } else {
      usleep(40 * 1000);
    }
    sem_post(&readSemaphore);
    iteration++;
  }
  return NULL;
}
