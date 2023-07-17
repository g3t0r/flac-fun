#include "../messages.h"

#include <assert.h>
#include <fcntl.h>
#include <stddef.h>
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

static void test__AlbumsMessageSizeCalculation() {
  char *albumNames[2] = {"Artist 1 - Album 1", "Artist 2 - Album 2"};

  struct AlbumListElement albumList[2] = {
      {1, strlen(albumNames[0]) + 1, albumNames[0]},
      {2, strlen(albumNames[1]) + 1, albumNames[1]},
  };

  struct AlbumsMessage message = {
      ALBUMS,   //
      0,        // value not important for calculation
      2,        //
      albumList //
  };

  int expectedSize = sizeof(message.type) +            //
                     sizeof(message.size) +            //
                     sizeof(message.numberOfAlbums) +  //
                     sizeof(albumList[0].albumId) +    //
                     sizeof(albumList[1].albumId) +    //
                     sizeof(albumList[0].nameLength) + //
                     sizeof(albumList[1].nameLength) + //
                     strlen(albumList[0].name) + 1 +   //
                     strlen(albumList[1].name) + 1;    //

  int calculatedSize = messageAlbumsGetSize(&message);

  assert(expectedSize == calculatedSize);
}

static void test__AlbumsMessageSerializationDeserialization(int writeFd,
                                                            int readFd) {

  char *albumNames[2] = {"Artist 1 - Album 1", "Artist 2 - Album 2"};

  struct AlbumListElement albumList[2] = {
      {1, strlen(albumNames[0]) + 1, albumNames[0]},
      {2, strlen(albumNames[1]) + 1, albumNames[1]},
  };

  struct AlbumsMessage toSerialize = {
      ALBUMS,   //
      0,        // value not important for calculation
      2,        //
      albumList //
  };

  serializeMessage(writeFd, (const struct Message *)&toSerialize);

  MessageSize calculatedSize = messageAlbumsGetSize(&toSerialize);

  struct AlbumsMessage *deserialized;
  deserializeMessage(readFd, (struct Message **)&deserialized);

  assert(toSerialize.type == deserialized->type);
  assert(calculatedSize == deserialized->size);
  assert(toSerialize.numberOfAlbums == deserialized->numberOfAlbums);

  for (size_t i = 0; i < toSerialize.numberOfAlbums; i++) {
    struct AlbumListElement *toSerializeAlbum = toSerialize.albumList + 1;
    struct AlbumListElement *deserializedAlbum = deserialized->albumList + 1;
    assert(toSerializeAlbum->albumId == deserializedAlbum->albumId);
    assert(toSerializeAlbum->nameLength == deserializedAlbum->nameLength);
    assert(strcmp(toSerializeAlbum->name, deserializedAlbum->name) == 0);
  }
}

static void test__SongsInAlbumMessageSizeCalculation() {
  struct SongListElement songList[3] = {
      {1, "Metallica - One", 60},
      {1, "Metallica - Two", 120},
      {1, "Metallica - Three", 180},
  };

  struct SongsInAlbumMessage message = {
      SONGS_IN_ALBUM,
      0, // real size no needed for calculation
      3, songList};

  size_t expectedSize = sizeof(message.type) +                //
                        sizeof(message.size) +                //
                        sizeof(message.numberOfSongs) +       //
                        sizeof(songList[0].songId) +          //
                        sizeof(songList[1].songId) +          //
                        sizeof(songList[2].songId) +          //
                        sizeof(songList[0].lengthInSeconds) + //
                        sizeof(songList[1].lengthInSeconds) + //
                        sizeof(songList[2].lengthInSeconds) + //
                        strlen(songList[0].name) + 1 +        //
                        strlen(songList[1].name) + 1 +        //
                        strlen(songList[2].name) + 1;         //

  MessageSize calculatedSize = messageSongsInAlbumSize(&message);
  assert(expectedSize == calculatedSize);
}

static void test__SongMetadataMessageSizeCalculation() {
  char buffer[8] = {'\x0', '\x1', '\x2', '\x3', '\x4', '\x5', '\x6', '\x7'};

  struct SongMetadataMessage message = {
      SONG_METADATA, //
      0,             //
      8,             //
      buffer         //
  };

  size_t expectedSize = sizeof(message.type) +            //
                        sizeof(message.size) +            //
                        sizeof(message.bytesSize) +       //
                        message.bytesSize * sizeof(char); //

  MessageSize calculatedSize = messageSongMetadataGetSize(&message);
  assert(expectedSize == calculatedSize);
}

static void test__SongAudioDataDataMessageSizeCalculation() {
  char buffer[8] = {'\x0', '\x1', '\x2', '\x3', '\x4', '\x5', '\x6', '\x7'};

  struct SongAudioDataMessage message = {
      SONG_METADATA, //
      0,             //
      8,             //
      buffer         //
  };

  size_t expectedSize = sizeof(message.type) +            //
                        sizeof(message.size) +            //
                        sizeof(message.bytesSize) +       //
                        message.bytesSize * sizeof(char); //

  MessageSize calculatedSize = messageSongAudioDataGetSize(&message);
  assert(expectedSize == calculatedSize);
}

int main() {

  int writeFd = creat("build/serialized.bin", S_IWUSR | S_IRUSR);
  int readFd = open("build/serialized.bin", S_IRUSR);

  test__DoListSongsInAlbumMessage__SerializationDeseralization(writeFd,
  readFd); test__DoListAlbumsMessage__SerializationDeseralization(writeFd,
  readFd);
  test__AlbumsMessageSizeCalculation();
  test__AlbumsMessageSerializationDeserialization(writeFd, readFd);
  test__SongsInAlbumMessageSizeCalculation();
  test__SongMetadataMessageSizeCalculation();
  test__SongAudioDataDataMessageSizeCalculation();
  printf("Messages: all test passed\n");

  int forBreakpoint = 0;
}
