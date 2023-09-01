#include "playback.h"
#include "bytes.h"
#include "circle-buffer.h"
#include "config.h"
#include "logs.h"
#include "messages.h"
#include <FLAC/stream_decoder.h>
#include <ao/ao.h>
#include <assert.h>
#include <bits/pthreadtypes.h>
#include <endian.h>
#include <pthread.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

static void *playback_audio_thread_fn(struct Playback *playback);

static FLAC__StreamDecoderWriteStatus flac_stream_decoder_write_cb(
    const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame,
    const FLAC__int32 *const *buffer, void *client_data);

static FLAC__StreamDecoderReadStatus
flac_stream_decoder_read_cb(const FLAC__StreamDecoder *decoder,
                            FLAC__byte buffer[], size_t *bytes,
                            void *client_data);

static void flac_stream_decoder_metadata_cb(const FLAC__StreamDecoder *decoder,
                                     const FLAC__StreamMetadata *metadata,
                                     void *client_data);

static void flac_stream_decoder_error_cb(const FLAC__StreamDecoder *decoder,
                                  FLAC__StreamDecoderErrorStatus status,
                                  void *client_data);




/* IMPLEMENTATION */

/* public */
int playback_init(struct Playback *playback) {

  // TODO: initialize semaphores
  ao_initialize();
  sem_init(&playback->semaphores.flac_data_mutex, 0, 1);
  sem_init(&playback->semaphores.flac_data_push, 0, FFUN_FLAC_DATA_BUFF_CAPACITY);
  sem_init(&playback->semaphores.flac_data_pull, 0, 0);
  sem_init(&playback->semaphores.raw_data_mutex, 0, 1);
  sem_init(&playback->semaphores.raw_data_push, 0, FFUN_FLAC_DATA_BUFF_CAPACITY);
  sem_init(&playback->semaphores.raw_data_pull, 0, 0);

  playback->flac_data_buffer = newCircleBuffer(FFUN_FLAC_DATA_BUFF_CAPACITY + 1,
                                             FFUN_FLAC_DATA_BUFF_ELEMENT_SIZE);

  playback->ao_info.driver = ao_default_driver_id();
  playback->decoder = FLAC__stream_decoder_new();
  FLAC__stream_decoder_init_stream(
      playback->decoder, flac_stream_decoder_read_cb, NULL, NULL, NULL, NULL,
      flac_stream_decoder_write_cb, flac_stream_decoder_metadata_cb,
      flac_stream_decoder_error_cb, playback);
  return 0;
}

int playback_start(struct Playback *playback) {

  pthread_t audio_thread;

  pthread_create(&audio_thread, NULL, (void *(*)(void *))playback_audio_thread_fn,
                 playback);

  FLAC__stream_decoder_process_until_end_of_stream(playback->decoder);

  pthread_join(audio_thread, NULL);
  printf("Thread joined, exiting\n");
  return 0;
}

void playback_feed_data(struct Playback * playback, char * data, size_t data_size) {
  sem_wait(&playback->semaphores.flac_data_push);
  sem_wait(&playback->semaphores.flac_data_mutex);

  struct CircleBufferEntry * entry =
    circle_buffer_write(playback->flac_data_buffer, data, data_size);
  assert(entry != NULL);

  sem_post(&playback->semaphores.flac_data_mutex);
  sem_post(&playback->semaphores.flac_data_pull);
}


/* private */
static void *playback_audio_thread_fn(struct Playback *playback) {

  printf("Starting audio playback loop\n");
  while (1) {
    sem_wait(&playback->semaphores.raw_data_pull);
    sem_wait(&playback->semaphores.raw_data_mutex);

    struct CircleBufferEntry *entry =
        circle_buffer_read(playback->raw_data_buffer);

    ao_play(playback->ao_info.device, entry->data, entry->size);

    sem_post(&playback->semaphores.raw_data_mutex);
    sem_post(&playback->semaphores.raw_data_push);
  }

  return NULL;
}

static FLAC__StreamDecoderWriteStatus flac_stream_decoder_write_cb(
    const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame,
    const FLAC__int32 *const *buffer, void *client_data) {

  struct Playback *playback = (struct Playback *)client_data;
  ao_sample_format *format = &playback->ao_info.format;
  uint32_t block_size = frame->header.blocksize;
  uint32_t number_of_channels = format->channels;
  uint32_t byte_mask = 0xFF;
  uint32_t bytes_per_sample = format->bits / 8;
  uint32_t buffer_size = block_size * bytes_per_sample * number_of_channels;

  pring_debug("Hello from flacWriteCb");
  char *output_buffer = calloc(buffer_size, sizeof(char));
  assert(output_buffer != NULL);

  for (int i = 0; i < block_size; i++) {
    for (int channel = 0; channel < number_of_channels; channel++) {

      int channel_offset =
          i * bytes_per_sample * number_of_channels + bytes_per_sample * channel;
      assert(buffer[channel] != NULL);

      /*
       * TODO
       * 3 times because 24 bits per sample (3*8)
       * Should make it more dynamic but now
       * it's not good time to do it
       */
      output_buffer[channel_offset] = (int8_t)(buffer[channel][i] >> 16) & byte_mask;
      output_buffer[channel_offset + 1] = (int8_t)(buffer[channel][i] >> 8) & byte_mask;
      output_buffer[channel_offset + 2] = (int8_t)buffer[channel][i] & byte_mask;
    }
  }

  sem_wait(&playback->semaphores.raw_data_push);
  sem_wait(&playback->semaphores.raw_data_mutex);

  circle_buffer_write(playback->raw_data_buffer, output_buffer, buffer_size);
  sem_post(&playback->semaphores.raw_data_mutex);
  sem_post(&playback->semaphores.raw_data_pull);

  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static FLAC__StreamDecoderReadStatus
flac_stream_decoder_read_cb(const FLAC__StreamDecoder *decoder,
                            FLAC__byte buffer[], size_t *bytes,
                            void *client_data) {

  struct Playback *playback = (struct Playback *)client_data;

  sem_wait(&playback->semaphores.flac_data_pull);
  sem_wait(&playback->semaphores.flac_data_mutex);

  const struct CircleBufferEntry *entry =
      circle_buffer_read(playback->flac_data_buffer);
  assert(entry != NULL);

  memcpy(buffer, entry->data, entry->size);
  *bytes = entry->size;
  pring_debug("Copied %lu bytes\n", *bytes);

  sem_post(&playback->semaphores.flac_data_mutex);
  sem_post(&playback->semaphores.flac_data_push);

  if (*bytes == 0)
    return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;

  return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

static void flac_stream_decoder_metadata_cb(const FLAC__StreamDecoder *decoder,
                                     const FLAC__StreamMetadata *metadata,
                                     void *client_data) {

  struct Playback *playback = (struct Playback *)client_data;
  ao_sample_format *format = &playback->ao_info.format;
  format->bits = metadata->data.stream_info.bits_per_sample;
  format->rate = metadata->data.stream_info.sample_rate;
  format->channels = metadata->data.stream_info.channels;
  format->byte_format = AO_FMT_BIG;

  playback->raw_data_buffer =
      newCircleBuffer(FFUN_RAW_DATA_BUFF_CAPACITY + 1,
                      metadata->data.stream_info.min_blocksize *
                          metadata->data.stream_info.bits_per_sample / 8 *
                          metadata->data.stream_info.channels);

  playback->ao_info.device = ao_open_live(playback->ao_info.driver, format, NULL);
}

static void flac_stream_decoder_error_cb(const FLAC__StreamDecoder *decoder,
                                  FLAC__StreamDecoderErrorStatus status,
                                  void *client_data) {
  printError("Stream decoder error, status: %d\n", status);
}

