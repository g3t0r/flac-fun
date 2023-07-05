#ifndef FFUN_MESSAGES_H_
#define FFUN_MESSAGES_H_

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
};

struct DoListAlbumsMessage {
  MessageType_8b type;
};

struct DoListSongsInAlbumsMessage {
  MessageType_8b type;
  uint16_t albumId;
};

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

#endif // FFUN_MESSAGES_H_
