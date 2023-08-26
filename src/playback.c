#include "playback.h"
#include "bytes.h"
#include "circle-buffer.h"
#include "config.h"
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
#include "logs.h"

static int semVal(sem_t *sem) {
  int v;
  sem_getvalue(sem, &v);
  return v;
}

#define dbg_sem_wait(sem)                                                      \
  printf("\tCalled sem_wait: %s:%d, value: %d\n", __FILE__, __LINE__,          \
         semVal(sem));                                                         \
  sem_wait(sem)

#define dbg_sem_post(sem)                                                      \
  printf("\tCalled sem_post: %s:%d, value: %d\n", __FILE__, __LINE__,          \
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

void *audioPlaybackFn(struct Playback *playback);

void *dataStreamFunc(struct Playback *playback);

/* IMPLEMENTATION */

int initPlayback(struct Playback *playback) {

  // TODO: initialize semaphores
  ao_initialize();
  sem_init(&playback->semaphores.flacData, 0, 1);
  sem_init(&playback->semaphores.pushFlacData, 0, FFUN_FLAC_DATA_BUFF_CAPACITY);
  sem_init(&playback->semaphores.pullFlacData, 0, 0);
  sem_init(&playback->semaphores.rawData, 0, 1);
  sem_init(&playback->semaphores.pushRawData, 0, FFUN_FLAC_DATA_BUFF_CAPACITY);
  sem_init(&playback->semaphores.pullRawData, 0, 0);

  playback->flacDataBuffer = newCircleBuffer(FFUN_FLAC_DATA_BUFF_CAPACITY + 1,
                                             FFUN_FLAC_DATA_BUFF_ELEMENT_SIZE);

  pthread_t dataStreamTid;
  pthread_create(&dataStreamTid, NULL, (void  * (*)(void *)) dataStreamFunc, playback);

  playback->aoInfo.driver = ao_default_driver_id();
  playback->decoder = FLAC__stream_decoder_new();
  FLAC__stream_decoder_init_stream(playback->decoder, flacReadCb, NULL, NULL,
                                   NULL, NULL, flacWriteCb, flacMetadataCb,
                                   flacErrorCb, playback);
  return 0;
}

int startPlayback(struct Playback *playback) {

  pthread_t audioPlaybackThread;

  pthread_create(&audioPlaybackThread, NULL, (void *(*)())audioPlaybackFn,
                 playback);

  FLAC__stream_decoder_process_until_end_of_stream(playback->decoder);

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

  printDebug("Hello from flacWriteCb");
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

  dbg_sem_wait(&playback->semaphores.pushRawData);
  sem_wait(&playback->semaphores.rawData);

  writeDataToBuffer(playback->rawDataBuffer, tmpBuffer, bufferSize);
  globalRawDataEntries++;
  sem_post(&playback->semaphores.rawData);
  sem_post(&playback->semaphores.pullRawData);

  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

FLAC__StreamDecoderReadStatus flacReadCb(const FLAC__StreamDecoder *decoder,
                                         FLAC__byte buffer[], size_t *bytes,
                                         void *clientData) {

  struct Playback *playback = (struct Playback *)clientData;

  sem_wait(&playback->semaphores.pullFlacData);
  sem_wait(&playback->semaphores.flacData);

  const struct CircleBufferEntry *entry =
      readEntryFromBuffer(playback->flacDataBuffer);
  assert(entry != NULL);
  memcpy(buffer, entry->data, entry->size);
  *bytes = entry->size;

  printDebug("Copied %lu bytes\n", *bytes);

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

  playback->rawDataBuffer = newCircleBuffer(FFUN_RAW_DATA_BUFF_CAPACITY + 1,
      metadata->data.stream_info.min_blocksize * metadata->data.stream_info.bits_per_sample / 8);
  

  playback->aoInfo.device = ao_open_live(playback->aoInfo.driver, format, NULL);
}

void *audioPlaybackFn(struct Playback *playback) {

  printf("Starting audio playback loop\n");
  while (1) {
    printDebug("RawDataPull: %d\n", semVal(&playback->semaphores.pullRawData));
    sem_wait(&playback->semaphores.pullRawData);
    sem_wait(&playback->semaphores.rawData);

    struct CircleBufferEntry *entry =
        readEntryFromBuffer(playback->rawDataBuffer);

    ao_play(playback->aoInfo.device, entry->data, entry->size);

    sem_post(&playback->semaphores.rawData);
    sem_post(&playback->semaphores.pushRawData);
  }

  return NULL;
}

void *dataStreamFunc(struct Playback *playback) {
  FILE * receivedDataFile = fopen("./audio/recieved.data.bin", "wb");
  FILE * partialDataFile = fopen("./audio/partial.data.bin", "wb");
  struct MessageHeader header;
  struct FeedMeMessage feedMeMessage;
  struct DataMessage dataMessage;
  int seq = 0;
  char udpDataBuffer[FFUN_UDP_DGRAM_MAX_SIZE];
  struct DataMessage partialResultsArray[FFUN_REQUESTED_ELEMENTS_NUMBER];
  char dataMergeArray[FFUN_REQUESTED_ELEMENTS_NUMBER * FFUN_REQUESTED_DATA_SIZE];

  struct pollfd pfd;
  pfd.fd = playback->socket;
  pfd.events = POLLIN;

  while (1) {
    sem_wait(&playback->semaphores.pushFlacData);
    sem_wait(&playback->semaphores.flacData);

    header.seq = FFUN_REQUESTED_ELEMENTS_NUMBER;
    header.type = FEED_ME;
    feedMeMessage.dataSize = FFUN_REQUESTED_DATA_SIZE;
    size_t offset = serializeMessageHeader(&header, udpDataBuffer);
    offset += serializeFeedMeMessage(&feedMeMessage, udpDataBuffer + offset);

    char memBackup[FFUN_UDP_DGRAM_MAX_SIZE];
    memcpy(memBackup, udpDataBuffer, FFUN_UDP_DGRAM_MAX_SIZE);

    printDebug("Sending FeedMe\n");

    send(playback->socket, udpDataBuffer, offset, 0);

    memset(partialResultsArray, 1, sizeof(partialResultsArray));

    for (int i = 0; i < FFUN_REQUESTED_ELEMENTS_NUMBER; i++) {
      int result = poll(&pfd, 1, FFUN_REQUESTED_DATA_TIMEOUT);

      if (result == -1) {
        printError("Poll error: %s\n", strerror(errno));
        exit(1);
      }

      if (result == 0 /* timeout */) {
        printError("poll: TIMEOUT\n");
        break;
      }


      size_t r = recv(playback->socket, udpDataBuffer, FFUN_UDP_DGRAM_MAX_SIZE, 0);
      assert(r != 9); // ?????? why is it 
      offset = deserializeMessageHeader(udpDataBuffer, &header);
      // printDebug("Received message with seq: %d\n", header.seq);
      assert(header.type == DATA);
      offset += deserializeDataMessage(udpDataBuffer + offset, &partialResultsArray[header.seq]);
      fwrite(partialResultsArray[header.seq].data, sizeof(char), partialResultsArray[header.seq].dataSize, receivedDataFile);
    }

    offset = 0;
    for(int i = 0; i < FFUN_REQUESTED_ELEMENTS_NUMBER; i++) {
      memcpy(&dataMergeArray[offset], partialResultsArray[i].data, partialResultsArray[i].dataSize);
      offset +=  partialResultsArray[i].dataSize;
    }
    writeDataToBuffer(playback->flacDataBuffer, dataMergeArray, offset);
    sem_post(&playback->semaphores.flacData);
    sem_post(&playback->semaphores.pullFlacData);
  }
}
