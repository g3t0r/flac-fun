#include "../messages.h"

#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static void test__DoListAlbumsMessage__SerializationDeseralization(int writeFd,
                                                                   int readFd) {
  struct DoListAlbumsMessage toSerialize;
  toSerialize.type = DO_LIST_ALBUMS;
  toSerialize.size = MESSAGE_DO_LIST_ALBUMS_SIZE;
  struct DoListAlbumsMessage *deserialized;

  serializeMessage(writeFd, (struct Message *)&toSerialize);
  deserializeMessage(readFd, (struct Message **)&deserialized);

  assert(toSerialize.type == deserialized->type);
  assert(toSerialize.size == deserialized->size);
}

static void
test__DoListSongsInAlbumMessage__SerializationDeseralization(int writeFd,
                                                             int readFd) {
  struct DoListSongsInAlbumsMessage toSerialize;
  toSerialize.type = DO_LIST_SONGS_IN_ALBUM;
  toSerialize.size = MESSAGE_DO_LIST_SONGS_IN_ALBUM_SIZE;
  toSerialize.albumId = 127;
  struct DoListSongsInAlbumsMessage *deserialized;

  serializeMessage(writeFd, (struct Message *)&toSerialize);
  deserializeMessage(readFd, (struct Message **)&deserialized);

  assert(toSerialize.type == deserialized->type);
  assert(toSerialize.albumId == deserialized->albumId);
  assert(toSerialize.size == deserialized->size);
}

static void test__AlbumsMessageGetSize() {
  struct AlbumListElement albumsList[2] = {
      {111, "Author 1 - Album Name 1"},
      {222, "Author 2 - Album Name 2"},
  };

  uint32_t albumsListSize = 2 * memberSize(struct AlbumListElement, albumId) +
                            strlen(albumsList[0].name) + 1 +
                            strlen(albumsList[1].name) + 1;

  struct AlbumsMessage albumsMessage = {ALBUMS, 0, 2, albumsList};

  uint32_t albumsMessageSize =
      memberSize(struct AlbumsMessage, type) +
      memberSize(struct AlbumsMessage, size) +
      memberSize(struct AlbumsMessage, numberOfAlbums) + albumsListSize;

  assert(albumsMessageSize == messageAlbumsGetSize(&albumsMessage));
}

int main() {

  int writeFd = creat("build/serialized.bin", S_IWUSR | S_IRUSR);
  int readFd = open("build/serialized.bin", S_IRUSR);

  test__DoListSongsInAlbumMessage__SerializationDeseralization(writeFd, readFd);
  test__DoListAlbumsMessage__SerializationDeseralization(writeFd, readFd);
  test__AlbumsMessageGetSize();
  printf("Messages: all test passed\n");

  int forBreakpoint = 0;
}
