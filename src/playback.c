#include "playback.h"
#include "circle-buffer.h"
#include <FLAC/stream_decoder.h>
#include <semaphore.h>
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

void * requestDataLoop(struct Playback * playback) {
  while(1) {
    sem_wait(&playback->produceSemaphore);

    char * data;
    size_t dataSize;
    playback->feedMeCb(&data, dataSize);
    writeDataToBuffer(playback->circleBuffer, data, dataSize);
    sem_post(&playback->consumeSemaphore);
  }
}

/* IMPLEMENTATION */

int initPlayback(struct Playback *playback) {

  playback->decoder = FLAC__stream_decoder_new();
  FLAC__stream_decoder_init_stream(playback->decoder, flacReadCb, NULL, NULL,
                                   NULL, NULL, flacWriteCb, flacMetadataCb,
                                   flacErrorCb, NULL);
  return 0;
}

int play(struct Playback *playback) {
  FLAC__stream_decoder_process_until_end_of_stream(playback->decoder);
  return 0;
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

  if(*bytes == 0)
    return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;

  return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}
