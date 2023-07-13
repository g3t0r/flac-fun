#ifndef _FFUN_MESSAGES_H_
#define _FFUN_MESSAGES_H_

#define memberSize(Type, member) (sizeof(((Type *)0)->member))

#include <stddef.h>
#include <stdint.h>

/* models and types */

enum MessageType {
  HEARTBEAT = 0,
  DO_LIST_ALBUMS,
  DO_LIST_SONGS_IN_ALBUM,
  ALBUMS,
  SONGS_IN_ALBUM
};

typedef uint8_t MessageType_8b;

static uint8_t toUint8(enum MessageType messageType);

struct Message {
  MessageType_8b type; // casted to uint8_t durring serialization
  uint32_t size;
};

extern const int MESSAGE_HEADER_SIZE;
struct DoListAlbumsMessage {
  MessageType_8b type;
  uint32_t size;
};

extern const int MESSAGE_DO_LIST_ALBUMS_SIZE;

struct DoListSongsInAlbumsMessage {
  MessageType_8b type;
  uint32_t size;
  uint16_t albumId;
};

extern const int MESSAGE_DO_LIST_SONGS_IN_ALBUM_SIZE;

struct AlbumListElement {
  uint16_t albumId;
  char *name;
};

uint32_t messageAlbumListElementGetSize(const struct AlbumListElement *message);

struct AlbumsMessage {
  MessageType_8b type;
  uint32_t size;
  uint32_t numberOfAlbums;
  struct AlbumListElement *albumList;
};

uint32_t messageAlbumsGetSize(const struct AlbumsMessage *message);

/* public functions */

int deserializeMessage(int fd, struct Message **dst);
int serializeMessage(int fd, const struct Message *message);

/* private methods */

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

#endif // _FFUN_MESSAGES_H_
