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

#include <FLAC/stream_decoder.h>
#include <ao/ao.h>
#include <ao/os_types.h>
#include <semaphore.h>
#include "circle-buffer.h"

struct AOInfo {
  ao_device * device;
  int default_driver;
  ao_sample_format format;
};

struct Playback {
  sem_t produceSemaphore;
  sem_t consumeSemaphore;
  struct CircleBuffer * circleBuffer;
  void (*feedMeCb)(char ** data, size_t dataSize);
  FLAC__StreamDecoder * decoder;
};

int initPlayback(struct Playback * playback);
int play(struct Playback * playback);

#endif
