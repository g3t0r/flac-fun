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
#include <unistd.h>

static int semVal(sem_t *sem) {
  int v;
  sem_getvalue(sem, &v);
  return v;
}

#define dbg_sem_wait(sem)                                                      \
  printf("\tCalled sem_wait: %s:%d, value: %d\n", __FILE__, __LINE__,            \
         semVal(sem));                                                         \
  sem_wait(sem)

#define dbg_sem_post(sem)                                                      \
  printf("\tCalled sem_post: %s:%d, value: %d\n", __FILE__, __LINE__,            \
         semVal(sem));                                                         \
  sem_post(sem)

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
  printf("Error: %d\n", status);
}

void *dataStreamFn(struct Playback *playback);
void *audioPlaybackFn(struct Playback *playback);

/* IMPLEMENTATION */

int initPlayback(struct Playback *playback) {

  // TODO: initialize semaphores
  sem_init(&playback->semaphores.flacData, 0, 1);
  sem_init(&playback->semaphores.pushFlacData, 0, FLAC_DATA_BUFFER_SIZE);
  sem_init(&playback->semaphores.pullFlacData, 0, 0);
  sem_init(&playback->semaphores.rawData, 0, 1);
  sem_init(&playback->semaphores.rawDataDelay, 0, 0);
  sem_init(&playback->semaphores.pushRawData, 0, FLAC_DATA_BUFFER_SIZE);
  sem_init(&playback->semaphores.pullRawData, 0, 0);

  playback->flacDataBuffer = newCircleBuffer(FLAC_DATA_BUFFER_SIZE + 1);
  playback->rawDataBuffer = newCircleBuffer(FLAC_DATA_BUFFER_SIZE + 1);
  playback->aoInfo.driver = ao_default_driver_id();
  playback->decoder = FLAC__stream_decoder_new();
  FLAC__stream_decoder_init_stream(playback->decoder, flacReadCb, NULL, NULL,
                                   NULL, NULL, flacWriteCb, flacMetadataCb,
                                   flacErrorCb, playback);
  return 0;
}

int play(struct Playback *playback) {
  pthread_t dataStreamThread;
  pthread_t dataStreamThread2;
  pthread_t audioPlaybackThread;

  pthread_create(&dataStreamThread, NULL, (void *(*)())dataStreamFn, playback);
  //pthread_create(&dataStreamThread2, NULL, (void *(*)())dataStreamFn, playback);

  pthread_create(&audioPlaybackThread, NULL, (void *(*)())audioPlaybackFn,
                 playback);

  FLAC__stream_decoder_process_until_end_of_stream(playback->decoder);


  pthread_join(dataStreamThread, NULL);
  pthread_join(dataStreamThread2, NULL);
  pthread_join(audioPlaybackThread, NULL);
  printf("Thread joined, exiting\n");
  return 0;
}

int globalRawDataEntries = 0;

FLAC__StreamDecoderWriteStatus flacWriteCb(const FLAC__StreamDecoder *decoder,
                                           const FLAC__Frame *frame,
                                           const FLAC__int32 *const *buffer,
                                           void *clientData) {
  struct Playback *playback = (struct Playback *)clientData;
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

      if (buffer[channel] == NULL) {
        int dbg = 0;
      }
      assert(buffer[channel] != NULL);
      int valueFromBuffer1 = *(*(buffer + channel) + i);
      int valueFromBuffer2 = buffer[channel][i];
      tmpBuffer[channelOffset] = (int8_t)(buffer[channel][i] >> 16) & byteMask;
      tmpBuffer[channelOffset + 1] =
          (int8_t)(buffer[channel][i] >> 8) & byteMask;
      tmpBuffer[channelOffset + 2] = (int8_t)buffer[channel][i] & byteMask;
    }
  }


  //printf("Write waiting\n");
  sem_wait(&playback->semaphores.pushRawData);
  sem_wait(&playback->semaphores.rawData);

  //printf("flacWriteCb\n");
  writeDataToBuffer(playback->rawDataBuffer, tmpBuffer, bufferSize);
  globalRawDataEntries++;
  sem_post(&playback->semaphores.rawData);
  sem_post(&playback->semaphores.pullRawData);

  //printf("Entries: %d\n", globalRawDataEntries);
  if(globalRawDataEntries == 50) {
    sem_post(&playback->semaphores.rawDataDelay);
  }


  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

FLAC__StreamDecoderReadStatus flacReadCb(const FLAC__StreamDecoder *decoder,
                                         FLAC__byte buffer[], size_t *bytes,
                                         void *clientData) {

  struct Playback *playback = (struct Playback *)clientData;

  //printf("waiting for read\n");
  //printf("FlacDataPull: %d\n", semVal(&playback->semaphores.pullFlacData));
  printf("FlacDataBufferCurrentSize: %lu\n", playback->flacDataBuffer->currentSize);
  sem_wait(&playback->semaphores.pullFlacData);
  sem_wait(&playback->semaphores.flacData);

  //printf("flacReadCb\n");
  struct CircleBufferEntry *entry =
      readEntryFromBuffer(playback->flacDataBuffer);
  assert(entry != NULL);
  memcpy(buffer, entry->data, entry->size);
  *bytes = entry->size;

  free(entry->data);
  entry->data = NULL;
  entry->size = 0;

  sem_post(&playback->semaphores.flacData);
  sem_post(&playback->semaphores.pushFlacData);

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

void *dataStreamFn(struct Playback *playback) {
  char *data = NULL;
  size_t dataSize;

  while (1) {
    //printf("Requesting flac data\n");
    playback->feedMeCb(playback->args, &data, &dataSize);
  }
}

void *audioPlaybackFn(struct Playback *playback) {

  printf("Starting audio playback loop\n");
  //sem_wait(&playback->semaphores.rawDataDelay);
  while (1) {
    printf("RawDataPull: %d\n", semVal(&playback->semaphores.pullRawData));
    sem_wait(&playback->semaphores.pullRawData);
    sem_wait(&playback->semaphores.rawData);

    struct CircleBufferEntry *entry =
        readEntryFromBuffer(playback->rawDataBuffer);

    assert(entry != NULL);
    assert(entry->size != 0);


    sem_post(&playback->semaphores.rawData);
    sem_post(&playback->semaphores.pushRawData);

    printf("Playing\n");
    ao_play(playback->aoInfo.device, entry->data, entry->size);

  }

  return NULL;
}
