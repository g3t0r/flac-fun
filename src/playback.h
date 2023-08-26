#ifndef __FFUN__PLAYBACK__
#define __FFUN__PLAYBACK__

/* ----- GENERAL IDEA -----
 *
 * In separate thread request new data from the server as needed.
 *    Would be the best to use external callback for it, to abstract
 *    data retrieval from the playback
 *
 * In main thread, process stream_decoder until end of the file.
 *    - Read callback should read from circle buffer feeded by
 *      the separate thread
 *
 *    - Metadata callback should save metadata as it will be required later
 *      by a write callback
 *
 *    - Write callback should convert flac decoded frame to raw audio data,
 *      and then play it using ao_play
 *
 *    NOTE: maybe i don't need callback, probably passing circle buffer alone
 *    would be enough
 *
 *    NEW IDEA: Inside playback: function working on separate thread, calling
 *              FeedMe callback which should return single buffer of data
 *
 *
 *
 * */

#include "circle-buffer.h"
#include <FLAC/stream_decoder.h>
#include <ao/ao.h>
#include <ao/os_types.h>
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

#define FLAC_DATA_BUFFER_SIZE 1000
#define RAW_DATA_BUFFER_SIZE 5

struct AOInfo {
  ao_device *device;
  int driver;
  ao_sample_format format;
  uint32_t blocksize;
};

struct Playback {
  struct Semaphores {
    sem_t flacData;
    sem_t pushFlacData;
    sem_t pullFlacData;
    sem_t rawData;
    sem_t pushRawData;
    sem_t pullRawData;
  } semaphores;
  struct CircleBuffer *flacDataBuffer;
  struct CircleBuffer *rawDataBuffer;
  struct AOInfo aoInfo;
  void (*feedMeCb)(void *args, char **data, size_t *dataSize);
  void *args;
  FLAC__StreamDecoder *decoder;
  int socket;
};

int initPlayback(struct Playback *playback);
int startPlayback(struct Playback *playback);

#endif
