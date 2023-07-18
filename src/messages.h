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
  SONGS_IN_ALBUM,
  DO_START_STREAM,
  DO_PAUSE_STREAM,
  DO_RESUME_STREAM,
  SONG_METADATA,
  SONG_AUDIO_DATA,
  READY_FOR_AUDIO
};

/* ==================== structs ==================== */

struct Message {
  MessageType_8b type;
  MessageSize size;
};

struct DoListAlbumsListMessage {
  MessageType_8b type;
  MessageSize size;
};

struct DoListSongsInAlbumsListMessage {
  MessageType_8b type;
  MessageSize size;
  AlbumId albumId;
};

struct AlbumListElement {
  uint16_t albumId;
  uint8_t nameLength;
  char *name;
};

struct AlbumsListMessage {
  MessageType_8b type;
  MessageSize size;
  uint32_t numberOfAlbums;
  struct AlbumListElement *albumList;
};

struct SongListElement {
  SongId songId;
  uint8_t nameLength;
  char *name;
  uint16_t lengthInSeconds;
};

struct SongsInAlbumMessage {
  MessageType_8b type;
  MessageSize size;
  uint8_t numberOfSongs;
  struct SongListElement *songList;
};

struct DoStartStreamMessage {
  MessageType_8b type;
  MessageSize size;
  SongId songId;
};

struct DoPauseStreamMessage {
  MessageType_8b type;
  MessageSize size;
};

struct DoResumeStreamMessage {
  MessageType_8b type;
  MessageSize size;
};

struct SongMetadataMessage {
  MessageType_8b type;
  MessageSize size;
  uint32_t bytesSize;
  char *bytes;
};

struct SongAudioDataMessage {
  MessageType_8b type;
  MessageSize size;
  uint32_t bytesSize;
  char *bytes;
};

/* =================== message sizes ==================== */

extern const int MESSAGE_HEADER_SIZE;

extern const int MESSAGE_DO_LIST_ALBUMS_SIZE;

extern const int MESSAGE_DO_LIST_SONGS_IN_ALBUM_SIZE;

extern const int MESSAGE_DO_START_STREAM_SIZE;

extern const int MESSAGE_DO_RESUME_STREAM_SIZE;

/* =================== deserialization functions ==================== */

int deserializeMessage(int fd, struct Message **dst);

/* =================== serialization functions ==================== */
int serializeMessage(int fd, const struct Message *message);

/* =============== size calculation functions================= */

MessageSize calculateMessageAlbumsListSize(const struct AlbumsListMessage *message);

MessageSize
calculateMessageSongsInAlbumSize(const struct SongsInAlbumMessage *message);

MessageSize
messageSongMetadataGetSize(const struct SongMetadataMessage *message);

MessageSize
messageSongAudioDataGetSize(const struct SongAudioDataMessage *message);

#endif // _FFUN_MESSAGES_H_
