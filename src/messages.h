#ifndef _FFUN_MESSAGES_H_
#define _FFUN_MESSAGES_H_

#define memberSize(Type, member) (sizeof(((Type *)0)->member))

#include <stddef.h>
#include <stdint.h>

/* =================== custom types ==================== */

typedef uint8_t MessageType_8b;
typedef uint32_t MessageSize;
typedef uint16_t AlbumId;
typedef uint8_t SongId;

enum MessageType {
  HEARTBEAT = 0,
  DO_LIST_ALBUMS,
  DO_LIST_SONGS_IN_ALBUM,
  ALBUMS,
  SONGS_IN_ALBUM
};

/* ==================== structs ==================== */

struct Message {
  MessageType_8b type;
  MessageSize size;
};

struct DoListAlbumsMessage {
  MessageType_8b type;
  MessageSize size;
};

struct DoListSongsInAlbumsMessage {
  MessageType_8b type;
  MessageSize size;
  AlbumId albumId;
};

struct AlbumListElement {
  uint16_t albumId;
  char *name;
};

struct AlbumsMessage {
  MessageType_8b type;
  MessageSize size;
  uint32_t numberOfAlbums;
  struct AlbumListElement *albumList;
};

struct SongListElement {
  SongId songId;
  char * name;
  uint16_t lengthInSeconds;
};

struct SongsInAlbumMessage {
  MessageType_8b type;
  MessageSize size;
  uint8_t numberOfSongs;
  struct SongListElement * songList;
};

/* =================== message sizes ==================== */

extern const int MESSAGE_HEADER_SIZE;

extern const int MESSAGE_DO_LIST_ALBUMS_SIZE;

extern const int MESSAGE_DO_LIST_SONGS_IN_ALBUM_SIZE;

/* =================== public functions ==================== */

int deserializeMessage(int fd, struct Message **dst);
int serializeMessage(int fd, const struct Message *message);

/* =================== private functions ==================== */

static uint32_t
messageAlbumListElementGetSize(const struct AlbumListElement *message);

static uint32_t messageAlbumsGetSize(const struct AlbumsMessage *message);

static int
serializeDoListAlbumsMessage(int fd, const struct DoListAlbumsMessage *message);

static int serializeDoListSongsInAlbumMessage(
    int fd, const struct DoListSongsInAlbumsMessage *message);

static int deserializeDoListAlbumsMessage(int fd,
                                          struct DoListAlbumsMessage **dst);

static int
deserializeDoListSongsInAlbumMessage(int fd,
                                     struct DoListSongsInAlbumsMessage **dst);

static int writeIntegerToBuffer(void *buff, const void *integer, size_t size);

static int readIntegerFromFile(int fd, void *integer, size_t size);

static int writeLoop(int fd, void *buffer, size_t bufferSize);

#endif // _FFUN_MESSAGES_H_
