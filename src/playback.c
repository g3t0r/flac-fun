#include "playback.h"
#include "circle-buffer.h"
#include <FLAC/stream_decoder.h>
#include <ao/ao.h>
#include <assert.h>
#include <endian.h>
#include <pthread.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

FLAC__StreamDecoderWriteStatus flacWriteCb(const FLAC__StreamDecoder *decoder,
                                           const FLAC__Frame *frame,
                                           const FLAC__int32 *const *buffer,
                                           void *clientData);

FLAC__StreamDecoderReadStatus flacReadCb(const FLAC__StreamDecoder *decoder,
                                         FLAC__byte buffer[], size_t *bytes,
                                         void *clientData);

void flacMetadataCb(const FLAC__StreamDecoder *decoder,
                    const FLAC__StreamMetadata *metadata, void *clientData);

void flacErrorCb(const FLAC__StreamDecoder *decoder,
                 FLAC__StreamDecoderErrorStatus status, void *clientData) {
  int dbg = 0;
}

void *requestDataLoop(struct Playback *playback);

/* IMPLEMENTATION */

int initPlayback(struct Playback *playback) {

  // TODO: initialize semaphores
  sem_init(&playback->semManipulation, 0, 1);
  sem_init(&playback->produceSemaphore, 0, CIRCLE_BUFFER_SIZE - 1);
  sem_init(&playback->consumeSemaphore, 0, 0);
  playback->circleBuffer = newCircleBuffer(CIRCLE_BUFFER_SIZE);
  playback->aoInfo.driver = ao_default_driver_id();
  playback->decoder = FLAC__stream_decoder_new();
  FLAC__stream_decoder_init_stream(playback->decoder, flacReadCb, NULL, NULL,
                                   NULL, NULL, flacWriteCb, flacMetadataCb,
                                   flacErrorCb, playback);
  return 0;
}

int play(struct Playback *playback) {
  pthread_create(&playback->requestDataLoopThread, NULL,
                 (void *(*)())requestDataLoop, playback);
  FLAC__stream_decoder_process_until_end_of_stream(playback->decoder);
  pthread_join(playback->requestDataLoopThread, NULL);
  printf("Thread joined, exiting\n");
  return 0;
}

FLAC__StreamDecoderWriteStatus flacWriteCb(const FLAC__StreamDecoder *decoder,
                                           const FLAC__Frame *frame,
                                           const FLAC__int32 *const *buffer,
                                           void *clientData) {
  struct Playback *playback = (struct Playback *)clientData;
  printf("Hello from writeCb\n");
  ao_sample_format *format = &playback->aoInfo.format;
  uint32_t blockSize = frame->header.blocksize;
  uint32_t bytesPerSample = format->bits / 8;
  uint32_t numberOfChannels = format->channels;
  uint32_t bufferSize = blockSize * bytesPerSample * numberOfChannels;
  uint32_t byteMask = 0xFF;

  char *tmpBuffer = calloc(bufferSize, sizeof(char));
  assert(tmpBuffer != NULL);


  for (int i = 0; i < blockSize; i++) {
    for (int channel = 0; channel < numberOfChannels; channel++) {

      int channelOffset =
          i * bytesPerSample * numberOfChannels + bytesPerSample * channel;

      if(buffer[channel] == NULL) {
        int dbg = 0;
      }
      assert(buffer[channel] != NULL);
      int valueFromBuffer1 = *(*(buffer+channel) + i);
      int valueFromBuffer2 = buffer[channel][i];
      tmpBuffer[channelOffset] = (int8_t)(buffer[channel][i] >> 16) & byteMask;
      tmpBuffer[channelOffset + 1] =
          (int8_t)(buffer[channel][i] >> 8) & byteMask;
      tmpBuffer[channelOffset + 2] = (int8_t)buffer[channel][i] & byteMask;
    }
  }

  struct AOInfo *aoInfo = &playback->aoInfo;

  printf("Before play\n");
  int playR = ao_play(aoInfo->device, tmpBuffer, bufferSize);
  printf("After play\n");

  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

size_t globalData = 0;

FLAC__StreamDecoderReadStatus flacReadCb(const FLAC__StreamDecoder *decoder,
                                         FLAC__byte buffer[], size_t *bytes,
                                         void *clientData) {

  struct Playback *playback = (struct Playback *)clientData;
  int abc = 0;
  printf("flacReadCb: waiting for consume semaphore\n");
  // NOTE: Could it be a deadlock?
  sem_wait(&playback->consumeSemaphore);
  sem_wait(&playback->semManipulation);
  printf("Reading from circle buffer\n");
  struct CircleBufferEntry *entry = readEntryFromBuffer(playback->circleBuffer);
  memcpy(buffer, entry->data, entry->size);
  *bytes = entry->size;
  *bytes += globalData;
  //printf("Read %lu bytes total\n", *bytes);

  free(entry->data);
  entry->data = NULL;
  entry->size = 0;

  sem_post(&playback->semManipulation);
  printf("flacReadCb: posting produce semaphore\n");
  sem_post(&playback->produceSemaphore);

  if (*bytes == 0)
    return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;

  return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

void flacMetadataCb(const FLAC__StreamDecoder *decoder,
                    const FLAC__StreamMetadata *metadata, void *clientData) {

  struct Playback *playback = (struct Playback *)clientData;
  ao_sample_format *format = &playback->aoInfo.format;
  format->bits = metadata->data.stream_info.bits_per_sample;
  format->rate = metadata->data.stream_info.sample_rate;
  format->channels = metadata->data.stream_info.channels;
  format->byte_format = AO_FMT_BIG;

  playback->aoInfo.device = ao_open_live(playback->aoInfo.driver, format, NULL);
}

void *requestDataLoop(struct Playback *playback) {
  int debug = 0;
  FILE * debugFile = fopen("./audio/output.debug.flac", "wb");
  printf("File error: %s\n", strerror(errno));
  while (1) {
    printf("requestDataLoop: waiting for produceSemaphore\n");
    sem_wait(&playback->produceSemaphore);
    sem_wait(&playback->semManipulation);
    char *data = NULL;
    size_t dataSize;
    playback->feedMeCb(playback->args, &data, &dataSize);
    fwrite(data, sizeof(char), dataSize, debugFile);
    writeDataToBuffer(playback->circleBuffer, data, dataSize);
    printf("requestDataLoop: posting for consumeSemaphore\n");
    sem_post(&playback->semManipulation);
    sem_post(&playback->consumeSemaphore);
    {
      int val = 0;
      sem_getvalue(&playback->consumeSemaphore, &val);
      printf("Semaphore value: %d\n", val);
    }
  }
}
