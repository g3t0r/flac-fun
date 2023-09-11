#ifndef __FFUN__PLAYBACK__
#define __FFUN__PLAYBACK__

#include <FLAC/stream_decoder.h>
#include <ao/ao.h>
#include <semaphore.h>
#include <stdint.h>

#include "circle-buffer.h"

struct AOInfo {
  ao_device *device;
  int driver;
  ao_sample_format format;
  uint32_t blocksize;
};

struct Playback {
  struct Semaphores {
    sem_t flac_data_mutex;
    sem_t flac_data_push;
    sem_t flac_data_pull;
    sem_t raw_data_mutex;
    sem_t raw_data_push;
    sem_t raw_data_pull;
    sem_t pause;
    sem_t pause_mutex;
  } semaphores;
  struct {
    pthread_t audio;
    pthread_t data_stream;
  } threads;
  struct CircleBuffer *flac_data_buffer;
  struct CircleBuffer *raw_data_buffer;
  struct AOInfo ao_info;
  void (*feedMeCb)(void *args, char **data, size_t *dataSize);
  void *args;
  FLAC__StreamDecoder *decoder;
  int socket;
  int playing;
  int stop;
};

int playback_init(struct Playback *playback);
int playback_start(struct Playback *playback);
void playback_feed_data(struct Playback * playback, char * data, size_t data_size);
void playback_pause(struct Playback * playback);
void playback_resume(struct Playback * playback);
void playback_stop(struct Playback * playback);

#endif
