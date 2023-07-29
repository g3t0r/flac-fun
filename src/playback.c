#include "playback.h"
#include "circle-buffer.h"
#include <FLAC/stream_decoder.h>
#include <ao/ao.h>
#include <endian.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
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
                 FLAC__StreamDecoderErrorStatus status, void *clientData) {}

void *requestDataLoop(struct Playback *playback);


/* IMPLEMENTATION */

int initPlayback(struct Playback *playback) {

  playback->aoInfo.driver = ao_default_driver_id();
  playback->decoder = FLAC__stream_decoder_new();
  FLAC__stream_decoder_init_stream(playback->decoder, flacReadCb, NULL, NULL,
                                   NULL, NULL, flacWriteCb, flacMetadataCb,
                                   flacErrorCb, playback);
  return 0;
}

int play(struct Playback *playback) {
  pthread_create(&playback->requestDataLoopThread, NULL, (void * (*)()) requestDataLoop, playback);
  FLAC__stream_decoder_process_until_end_of_stream(playback->decoder);
  pthread_join(playback->requestDataLoopThread, NULL);
  return 0;
}

FLAC__StreamDecoderWriteStatus flacWriteCb(const FLAC__StreamDecoder *decoder,
                                           const FLAC__Frame *frame,
                                           const FLAC__int32 *const *buffer,
                                           void *clientData) {
  struct Playback *playback = (struct Playback *)clientData;
  ao_sample_format *format = &playback->aoInfo.format;
  uint32_t blockSize = playback->aoInfo.blocksize;
  uint32_t bytesPerSample = format->bits / 8;
  uint32_t numberOfChannels = format->channels;
  uint32_t bufferSize = blockSize * bytesPerSample * numberOfChannels;
  uint32_t byteMask = 0xFF;

  char *tmpBuffer = calloc(bufferSize, sizeof(char));

  for (int i = 0; i < blockSize; i++) {
    for (int channel = 0; channel < numberOfChannels; channel++) {

      int channelOffset =
          i * bytesPerSample * numberOfChannels + bytesPerSample * channel;

      tmpBuffer[channelOffset] = (int8_t)(buffer[channel][i] >> 16) & byteMask;
      tmpBuffer[channelOffset + 1] =
          (int8_t)(buffer[channel][i] >> 8) & byteMask;
      tmpBuffer[channelOffset + 2] = (int8_t)buffer[channel][i] & byteMask;
    }
  }

  struct AOInfo * aoInfo = &playback->aoInfo;

  int playR = ao_play(aoInfo->device, tmpBuffer, bufferSize);

  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

FLAC__StreamDecoderReadStatus flacReadCb(const FLAC__StreamDecoder *decoder,
                                         FLAC__byte buffer[], size_t *bytes,
                                         void *clientData) {

  struct Playback *playback = (struct Playback *)clientData;

  struct CircleBufferEntry *entry = readEntryFromBuffer(playback->circleBuffer);
  memcpy(buffer, entry->data, entry->size);
  *bytes = entry->size;

  free(entry->data);
  entry->data = NULL;
  entry->size = 0;

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

  playback->aoInfo.device =
      ao_open_live(playback->aoInfo.driver, format, NULL);
}

void *requestDataLoop(struct Playback *playback) {
  while (1) {
    sem_wait(&playback->produceSemaphore);
    char *data;
    size_t dataSize;
    playback->feedMeCb(playback->args, &data, dataSize);
    writeDataToBuffer(playback->circleBuffer, data, dataSize);
    sem_post(&playback->consumeSemaphore);
  }
}
