#include "../messages.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdio.h>

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

int main() {

  int writeFd = creat("build/serialized.bin", S_IWUSR | S_IRUSR);
  int readFd = open("build/serialized.bin", S_IRUSR);

  test__DoListSongsInAlbumMessage__SerializationDeseralization(writeFd, readFd);
  test__DoListAlbumsMessage__SerializationDeseralization(writeFd, readFd);
  printf("Messages: all test passed\n");

  int forBreakpoint = 0;
}
