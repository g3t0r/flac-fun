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
calculateMessageAlbumListElementSize(const struct AlbumListElement *message);

static MessageSize
calculateMessageSongsListElementSize(const struct SongListElement *song);

static int
serializeDoListAlbumsMessage(int fd, const struct DoListAlbumsListMessage *message);

static int serializeDoListSongsInAlbumMessage(
    int fd, const struct DoListSongsInAlbumsListMessage *message);

static int
serializeDoListAlbumsMessage(int fd, const struct DoListAlbumsListMessage *message);

static int serializeDoListSongsInAlbumMessage(
    int fd, const struct DoListSongsInAlbumsListMessage *message);

static int serializeAlbumsListMessage(int fd, const struct AlbumsListMessage *message);

static int serializeSongsInAlbumMessage(int fd,
                                        struct SongsInAlbumMessage *dst);

static int deserializeDoListAlbumsMessage(int fd,
                                          struct DoListAlbumsListMessage **dst);

static int
deserializeDoListSongsInAlbumMessage(int fd,
                                     struct DoListSongsInAlbumsListMessage **dst);

static int deserializeAlbumsListMessage(int fd, struct AlbumsListMessage **dst);

static int writeIntegerToBuffer(void *buff, const void *integer, size_t size);

static int writeStringToBuffer(void *buff, const char *string);

static int writeLoop(int fd, void *buffer, size_t bufferSize);

static int readIntegerFromFile(int fd, void *integer, size_t size);

static int readStringFromFile(int fd, char **string, uint8_t length);

/* ===================| definitions |==================== */

/* =================== message sizes ==================== */

const int MESSAGE_HEADER_SIZE =
    memberSize(struct Message, type) + memberSize(struct Message, size);

const int MESSAGE_DO_LIST_ALBUMS_SIZE = MESSAGE_HEADER_SIZE;

const int MESSAGE_DO_LIST_SONGS_IN_ALBUM_SIZE =
    MESSAGE_HEADER_SIZE +
    memberSize(struct DoListSongsInAlbumsListMessage, albumId);

/* =================== public functions ==================== */

/* =================== serialization ==================== */
int serializeMessage(int fd, const struct Message *message) {
  switch (message->type) {
  case DO_LIST_ALBUMS:
    return serializeDoListAlbumsMessage(fd,
                                        (struct DoListAlbumsListMessage *)message);
  case DO_LIST_SONGS_IN_ALBUM:
    return serializeDoListSongsInAlbumMessage(
        fd, (struct DoListSongsInAlbumsListMessage *)message);
  case ALBUMS:
    return serializeAlbumsListMessage(fd, (struct AlbumsListMessage *)message);
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

  switch ((enum MessageType)(*dst)->type) {
  case DO_LIST_ALBUMS:
    return deserializeDoListAlbumsMessage(fd,
                                          (struct DoListAlbumsListMessage **)dst);
  case DO_LIST_SONGS_IN_ALBUM:
    return deserializeDoListSongsInAlbumMessage(
        fd, (struct DoListSongsInAlbumsListMessage **)dst);
  case ALBUMS:
    return deserializeAlbumsListMessage(fd, (struct AlbumsListMessage **)dst);
  default:
    return -1;
  }
}

/* =================== message size calculation ==================== */

MessageSize calculateMessageAlbumsListSize(const struct AlbumsListMessage *message) {
  int size = MESSAGE_HEADER_SIZE;
  size += sizeof(message->numberOfAlbums);
  for (int i = 0; i < message->numberOfAlbums; i++) {
    size += calculateMessageAlbumListElementSize(message->albumList + i);
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

MessageSize
calculateMessageSongsInAlbumSize(const struct SongsInAlbumMessage *message) {
  int size = MESSAGE_HEADER_SIZE;
  size += sizeof(message->numberOfSongs);
  for (int i = 0; i < message->numberOfSongs; i++) {
    size += calculateMessageSongsListElementSize(message->songList + i);
  }
  return size;
}

/* =================== private functions ==================== */

/* ============ message size calculation functions ============= */

static MessageSize
calculateMessageAlbumListElementSize(const struct AlbumListElement *message) {
  return sizeof(message->albumId) +          //
         sizeof(message->nameLength) +       //
         sizeof(char) * message->nameLength; //
}

static MessageSize
calculateMessageSongsListElementSize(const struct SongListElement *song) {
  return sizeof(song->songId) +            //
         sizeof(song->nameLength) +        //
         sizeof(char) * song->nameLength + //
         sizeof(song->lengthInSeconds);    //
}

/* =================== serialization functions ==================== */
static int
serializeDoListAlbumsMessage(int fd,
                             const struct DoListAlbumsListMessage *message) {

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
    int fd, const struct DoListSongsInAlbumsListMessage *message) {

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

static int serializeAlbumsListMessage(int fd, const struct AlbumsListMessage *message) {

  MessageSize total = calculateMessageAlbumsListSize(message);
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

    inBuffer += writeIntegerToBuffer(buffer + inBuffer,          //
                                     &album->nameLength,         //
                                     sizeof(album->nameLength)); //

    inBuffer += writeStringToBuffer(buffer + inBuffer, album->name);
  }

  writeLoop(fd, buffer, total);

  return 0;
}

static int serializeSongsInAlbumMessage(int fd,
                                        struct SongsInAlbumMessage *message) {
  MessageSize total = calculateMessageSongsInAlbumSize(message);
  char *buffer = malloc(sizeof(char) * total);

  MessageSize inBuffer = 0;
  inBuffer += writeIntegerToBuffer(buffer + inBuffer,      //
                                   &message->type,         //
                                   sizeof(message->type)); //

  inBuffer += writeIntegerToBuffer(buffer + inBuffer,      //
                                   &total,                 //
                                   sizeof(message->size)); //

  inBuffer += writeIntegerToBuffer(buffer + inBuffer,               //
                                   &message->numberOfSongs,         //
                                   sizeof(message->numberOfSongs)); //

  for (size_t i = 0; i < message->numberOfSongs; i++) {
    struct SongListElement *song = message->songList + i;

    inBuffer += writeIntegerToBuffer(buffer + inBuffer,     //
                                     &song->songId,         //
                                     sizeof(song->songId)); //

    inBuffer += writeIntegerToBuffer(buffer + inBuffer,         //
                                     &song->nameLength,         //
                                     sizeof(song->nameLength)); //

    inBuffer += writeStringToBuffer(buffer + inBuffer, song->name);

    inBuffer += writeIntegerToBuffer(buffer + inBuffer,              //
                                     &song->lengthInSeconds,         //
                                     sizeof(song->lengthInSeconds)); //
  }

  writeLoop(fd, buffer, total);
  return 0;
}

/* =================== deserialization functions ==================== */

static int deserializeDoListAlbumsMessage(int fd,
                                          struct DoListAlbumsListMessage **dst) {
  return 0;
}

static int
deserializeDoListSongsInAlbumMessage(int fd,
                                     struct DoListSongsInAlbumsListMessage **dst) {

  readIntegerFromFile(fd, &(*dst)->albumId, sizeof((*dst)->albumId));

  return 0;
}

static int deserializeAlbumsListMessage(int fd, struct AlbumsListMessage **dst) {
  int staticSize = sizeof(struct AlbumsListMessage);
  *dst = realloc(*dst, sizeof(char) * staticSize);
  if (*dst == NULL) {
    printf("Memory error: %s\n", strerror(errno));
    exit(1);
  }

  readIntegerFromFile(fd, &(*dst)->numberOfAlbums,
                      sizeof((*dst)->numberOfAlbums));
  (*dst)->albumList =
      malloc((*dst)->numberOfAlbums * sizeof(struct AlbumListElement));

  for (size_t i = 0; i < (*dst)->numberOfAlbums; i++) {
    struct AlbumListElement *element = (*dst)->albumList + i;
    readIntegerFromFile(fd, &element->albumId, sizeof(element->albumId));
    readIntegerFromFile(fd, &element->nameLength, sizeof(element->nameLength));
    readStringFromFile(fd, &element->name, element->nameLength);
  }

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

static int readStringFromFile(int fd, char **string, uint8_t length) {
  *string = malloc(sizeof(char) * length);
  readLoop(fd, *string, length);
  return 0;
}

/* =================== write functions ==================== */

static int writeStringToBuffer(void *buff, const char *string) {
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
