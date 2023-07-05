#include "messages.h"
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

// for tests

#ifndef TESTMESSAGES
#else
#include <fcntl.h>
#include <sys/stat.h>
#endif

int serializeMessage(int fd, const struct Message *message) {
  switch (message->type) {
  case DO_LIST_ALBUMS: {
    return serializeDoListAlbumsMessage(fd,
                                        (struct DoListAlbumsMessage *)message);
  case DO_LIST_SONGS_IN_ALBUM:
    return serializeDoListSongsInAlbumMessage(
        fd, (struct DoListSongsInAlbumsMessage *)message);
  }
  default:
    return -1;
  };
}


int deserializeMessage(int fd, struct Message **dst) {
  *dst = malloc(sizeof(struct Message));
  int readBytes = read(fd, &(*dst)->type, sizeof(MessageType_8b));
  if (readBytes == -1) {
    return -1;
  }

  switch ((enum MessageType)(*dst)->type) {
  case DO_LIST_ALBUMS:
    return deserializeDoListAlbumsMessage(fd,
                                          (struct DoListAlbumsMessage **)dst);
  case DO_LIST_SONGS_IN_ALBUM:
    return deserializeDoListSongsInAlbumMessage(
        fd, (struct DoListSongsInAlbumsMessage **)dst);
  default:
    return -1;
  }
}

static int writeLoop(int fd, void *buffer, size_t bufferSize);


static int
serializeDoListAlbumsMessage(int fd,
                             const struct DoListAlbumsMessage *message) {

  int total = sizeof(message->type);
  char *buffer = malloc(sizeof(char) * total);
  writeIntegerToBuffer(buffer, &message->type, sizeof(message->type));
  writeLoop(fd, buffer, total);
  free(buffer);

  return 0;
}

static int serializeDoListSongsInAlbumMessage(
    int fd, const struct DoListSongsInAlbumsMessage *message) {

  int total = sizeof(message->type) + sizeof(message->albumId);
  char *buffer = malloc(sizeof(char) * total);
  int inBuffer = 0;

  inBuffer += writeIntegerToBuffer(buffer + inBuffer, &message->type,
                                   sizeof(message->type));
  inBuffer += writeIntegerToBuffer(buffer + inBuffer, &message->albumId,
                                   sizeof(message->albumId));
  writeLoop(fd, buffer, total);
  free(buffer);

  return 0;
}

static int deserializeDoListAlbumsMessage(int fd,
                                          struct DoListAlbumsMessage **dst) {
  *dst = realloc(*dst, sizeof(struct DoListAlbumsMessage));
  if (dst == 0) {
    return -1;
  }
  return 0;
}

static int
deserializeDoListSongsInAlbumMessage(int fd,
                                     struct DoListSongsInAlbumsMessage **dst) {
  *dst = realloc(*dst, sizeof(struct DoListSongsInAlbumsMessage));

  if (*dst == 0) {
    return -1;
  }

  readIntegerFromFile(fd, &(*dst)->albumId, sizeof((*dst)->albumId));

  return 0;
}

static int writeIntegerToBuffer(void *buff, const void *integer, size_t size) {
  if (size == sizeof(uint8_t)) {
    memcpy(buff, integer, size);
  } else if (size == sizeof(uint16_t)) {
    uint16_t networkByteOrder = htons(*(const uint16_t *)integer);
    memcpy(buff, &networkByteOrder, size);
  } else {
    assert(sizeof(uint32_t) == size);
    uint32_t networkByteOrder = htonl(*(const uint32_t *)integer);
    memcpy(buff, &networkByteOrder, size);
  }
  return size;
}


static int readLoop(int fd, void * buffer, size_t bufferSize) {
  int bytesRead = 0;
  while((read(fd, buffer + bytesRead, bufferSize - bytesRead)) != 0) {
    if(bytesRead == -1) {
      return -1;
    }
  }
  return 0;
}

static int readIntegerFromFile(int fd, void *integer, size_t size) {
  int readBytes = 0;
  if (size == sizeof(uint8_t)) {

     readBytes = read(fd, integer, size);

  } else if (size == sizeof(uint16_t)) {

    uint16_t networkByteOrder;
    readLoop(fd, &networkByteOrder, size);
    *(uint16_t *)integer = ntohs(networkByteOrder);

  } else {

    assert(sizeof(uint32_t) == size);
    uint32_t netwotkByteOrder;
    readLoop(fd, &netwotkByteOrder, size);
    *(uint32_t *)integer = ntohl(netwotkByteOrder);

  }
  return size;
}


static int writeLoop(int fd, void *buffer, size_t bufferSize) {
  int bytesWritten = 0;
  while ((bytesWritten = write(fd, buffer + bytesWritten,
                               bufferSize - bytesWritten)) != 0) {
    if (bytesWritten == -1) {
      printf("%s\n", strerror(errno));
      return -1;
    }
  }
  return 0;
}

static uint8_t toUint8(enum MessageType messageType) {
  return (uint8_t)messageType;
}

#ifndef TESTMESSAGES
#else

static void test__DoListAlbumsMessage__SerializationDeseralization(int writeFd, int readFd) {
  struct DoListAlbumsMessage message;
  message.type = DO_LIST_ALBUMS;
  struct DoListAlbumsMessage *deserialized;

  serializeMessage(writeFd, (struct Message *)&message);
  deserializeMessage(readFd, (struct Message **)&deserialized);

  assert(message.type == deserialized->type);
}

static void test__DoListSongsInAlbumMessage__SerializationDeseralization(int writeFd, int readFd) {
  struct DoListSongsInAlbumsMessage toSerialize;
  toSerialize.type = DO_LIST_SONGS_IN_ALBUM;
  toSerialize.albumId = 212;
  struct DoListSongsInAlbumsMessage *deserialized;

  serializeMessage(writeFd, (struct Message *)&toSerialize);
  deserializeMessage(readFd, (struct Message **)&deserialized);

  assert(toSerialize.type == deserialized->type);
  assert(toSerialize.albumId == deserialized->albumId);
}


int main() {

  int writeFd = creat("build/serialized.bin", S_IWUSR | S_IRUSR);
  int readFd = open("build/serialized.bin", S_IRUSR);


  test__DoListSongsInAlbumMessage__SerializationDeseralization(writeFd, readFd);
  test__DoListAlbumsMessage__SerializationDeseralization(writeFd, readFd);



  int forBreakpoint = 0;
}

#endif
