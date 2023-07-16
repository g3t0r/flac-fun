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

/* =================== static function declarations ==================== */

static MessageSize
messageAlbumListElementGetSize(const struct AlbumListElement *message);

static MessageSize
messageSongsListElementGetSize(const struct SongListElement *song);

static int
serializeDoListAlbumsMessage(int fd, const struct DoListAlbumsMessage *message);

static int serializeDoListSongsInAlbumMessage(
    int fd, const struct DoListSongsInAlbumsMessage *message);

static int
serializeDoListAlbumsMessage(int fd, const struct DoListAlbumsMessage *message);

static int serializeDoListSongsInAlbumMessage(
    int fd, const struct DoListSongsInAlbumsMessage *message);

static int serializeAlbumsMessage(int fd, const struct AlbumsMessage *message);

static int deserializeDoListAlbumsMessage(int fd,
                                          struct DoListAlbumsMessage **dst);

static int
deserializeDoListSongsInAlbumMessage(int fd,
                                     struct DoListSongsInAlbumsMessage **dst);

static int writeIntegerToBuffer(void *buff, const void *integer, size_t size);

static int writeStringToBuffer(void *buff, const char * string);

static int writeLoop(int fd, void *buffer, size_t bufferSize);

static int readIntegerFromFile(int fd, void *integer, size_t size);

/* ===================| definitions |==================== */

/* =================== message sizes ==================== */

const int MESSAGE_HEADER_SIZE =
    memberSize(struct Message, type) + memberSize(struct Message, size);

const int MESSAGE_DO_LIST_ALBUMS_SIZE = MESSAGE_HEADER_SIZE;

const int MESSAGE_DO_LIST_SONGS_IN_ALBUM_SIZE =
    MESSAGE_HEADER_SIZE +
    memberSize(struct DoListSongsInAlbumsMessage, albumId);

/* =================== public functions ==================== */

/* =================== serialization ==================== */
int serializeMessage(int fd, const struct Message *message) {
  switch (message->type) {
  case DO_LIST_ALBUMS:
    return serializeDoListAlbumsMessage(fd,
                                        (struct DoListAlbumsMessage *)message);
  case DO_LIST_SONGS_IN_ALBUM:
    return serializeDoListSongsInAlbumMessage(
        fd, (struct DoListSongsInAlbumsMessage *)message);
  case ALBUMS:
    return 0;
  default:
    return -1;
  };
}

/* =================== derialization ==================== */
int deserializeMessage(int fd, struct Message **dst) {
  *dst = malloc(sizeof(struct Message));

  int readBytes = read(fd, &(*dst)->type, sizeof(MessageType_8b));
  if (readBytes == -1) {
    return -1;
  }

  readBytes = readIntegerFromFile(fd, &(*dst)->size, sizeof((*dst)->size));
  if (readBytes == -1) {
    return -1;
  }

  *dst = realloc(*dst, (*dst)->size * sizeof(char));
  if (*dst == NULL) {
    printf("Memory error: %s\n", strerror(errno));
    exit(1);
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

/* =================== message size calculation ==================== */

MessageSize messageAlbumsGetSize(const struct AlbumsMessage *message) {
  int size = MESSAGE_HEADER_SIZE;
  size += sizeof(message->numberOfAlbums);
  for (int i = 0; i < message->numberOfAlbums; i++) {
    size += messageAlbumListElementGetSize(message->albumList + i);
  }
  return size;
}

MessageSize
messageSongMetadataGetSize(const struct SongMetadataMessage *message) {
  return MESSAGE_HEADER_SIZE + sizeof(message->bytesSize) +
         message->bytesSize * sizeof(char);
}

MessageSize
messageSongAudioDataGetSize(const struct SongAudioDataMessage *message) {
  return MESSAGE_HEADER_SIZE + sizeof(message->bytesSize) +
         message->bytesSize * sizeof(char);
}

MessageSize messageSongsInAlbumSize(const struct SongsInAlbumMessage *message) {
  int size = MESSAGE_HEADER_SIZE;
  size += sizeof(message->numberOfSongs);
  for (int i = 0; i < message->numberOfSongs; i++) {
    size += messageSongsListElementGetSize(message->songList + i);
  }
  return size;
}

/* =================== private functions ==================== */

/* ============ message size calculation functions ============= */

static MessageSize
messageAlbumListElementGetSize(const struct AlbumListElement *message) {
  return sizeof(message->albumId) + strlen(message->name) + 1;
}

static MessageSize
messageSongsListElementGetSize(const struct SongListElement *song) {
  return sizeof(song->songId) + sizeof(song->lengthInSeconds) +
         strlen(song->name) + 1;
}

/* =================== serialization functions ==================== */
static int
serializeDoListAlbumsMessage(int fd,
                             const struct DoListAlbumsMessage *message) {

  int total = MESSAGE_DO_LIST_ALBUMS_SIZE;
  char *buffer = malloc(sizeof(char) * total);
  size_t inBuffer = 0;
  inBuffer += writeIntegerToBuffer(buffer + inBuffer, &message->type,
                                   sizeof(message->type));

  inBuffer += writeIntegerToBuffer(buffer + inBuffer, &message->size,
                                   sizeof(message->size));
  writeLoop(fd, buffer, total);
  free(buffer);

  return 0;
}

static int serializeDoListSongsInAlbumMessage(
    int fd, const struct DoListSongsInAlbumsMessage *message) {

  uint32_t total = MESSAGE_DO_LIST_SONGS_IN_ALBUM_SIZE;
  char *buffer = malloc(sizeof(char) * total);
  int inBuffer = 0;

  inBuffer += writeIntegerToBuffer(buffer + inBuffer, &message->type,
                                   sizeof(message->type));

  inBuffer += writeIntegerToBuffer(buffer + inBuffer, &total, sizeof(uint32_t));

  inBuffer += writeIntegerToBuffer(buffer + inBuffer, &message->albumId,
                                   sizeof(message->albumId));
  writeLoop(fd, buffer, total);
  free(buffer);

  return 0;
}

static int serializeAlbumsMessage(int fd, const struct AlbumsMessage *message) {

  MessageSize total = messageAlbumsGetSize(message);
  char *buffer = malloc(sizeof(char) * total);
  MessageSize inBuffer = 0;

  inBuffer += writeIntegerToBuffer(buffer + inBuffer,      //
                                   &message->type,         //
                                   sizeof(message->type)); //

  inBuffer += writeIntegerToBuffer(buffer + inBuffer,    //
                                   &total,               //
                                   sizeof(MessageSize)); //

  inBuffer += writeIntegerToBuffer(buffer + inBuffer,                //
                                   &message->numberOfAlbums,         //
                                   sizeof(message->numberOfAlbums)); //

  for (uint32_t i = 0; i < message->numberOfAlbums; i++) {
    struct AlbumListElement *album = message->albumList + i;

    inBuffer += writeIntegerToBuffer(buffer + inBuffer, //
                                     &album->albumId,   //
                                     sizeof(AlbumId));  //

    inBuffer += writeStringToBuffer(buffer + inBuffer, album->name);
  }

  return 0;
}

/* =================== deserialization functions ==================== */

static int deserializeDoListAlbumsMessage(int fd,
                                          struct DoListAlbumsMessage **dst) {
  return 0;
}

static int
deserializeDoListSongsInAlbumMessage(int fd,
                                     struct DoListSongsInAlbumsMessage **dst) {

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

/* =================== read functions ==================== */

static int readLoop(int fd, void *buffer, size_t bufferSize) {
  int bytesRead = 0;
  while ((bytesRead = read(fd, buffer + bytesRead, bufferSize - bytesRead)) !=
         0) {
    if (bytesRead == -1) {
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
    int breakpointPlaceholder = 0;
  }
  return size;
}

/* =================== write functions ==================== */

static int writeStringToBuffer(void *buff, const char * string) {
  size_t length = strlen(string) + 1;
  memcpy(buff, string, length);
  return length;
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
