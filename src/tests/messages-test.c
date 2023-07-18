#include "../messages.h"

#include <assert.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static void
test__DoListAlbumsListMessage__SerializationDeseralization(int writeFd,
                                                           int readFd) {
  struct DoListAlbumsListMessage toSerialize;
  toSerialize.type = DO_LIST_ALBUMS;
  toSerialize.size = MESSAGE_DO_LIST_ALBUMS_SIZE;
  struct DoListAlbumsListMessage *deserialized;

  serializeMessage(writeFd, (struct Message *)&toSerialize);
  deserializeMessage(readFd, (struct Message **)&deserialized);

  assert(toSerialize.type == deserialized->type);
  assert(toSerialize.size == deserialized->size);
}

static void
test__DoListSongsInAlbumMessage__SerializationDeseralization(int writeFd,
                                                             int readFd) {
  struct DoListSongsInAlbumsListMessage toSerialize;
  toSerialize.type = DO_LIST_SONGS_IN_ALBUM;
  toSerialize.size = MESSAGE_DO_LIST_SONGS_IN_ALBUM_SIZE;
  toSerialize.albumId = 127;
  struct DoListSongsInAlbumsListMessage *deserialized;

  serializeMessage(writeFd, (struct Message *)&toSerialize);
  deserializeMessage(readFd, (struct Message **)&deserialized);

  assert(toSerialize.type == deserialized->type);
  assert(toSerialize.albumId == deserialized->albumId);
  assert(toSerialize.size == deserialized->size);
}

static void test__AlbumsListMessageSizeCalculation() {
  char *albumNames[2] = {"Artist 1 - Album 1", "Artist 2 - Album 2"};

  struct AlbumListElement albumList[2] = {
      {1, strlen(albumNames[0]) + 1, albumNames[0]},
      {2, strlen(albumNames[1]) + 1, albumNames[1]},
  };

  struct AlbumsListMessage message = {
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

  int calculatedSize = calculateMessageAlbumsListSize(&message);

  assert(expectedSize == calculatedSize);
}

static void test__AlbumsListMessageSerializationDeserialization(int writeFd,
                                                                int readFd) {

  char *albumNames[2] = {"Artist 1 - Album 1", "Artist 2 - Album 2"};

  struct AlbumListElement albumList[2] = {
      {1, strlen(albumNames[0]) + 1, albumNames[0]},
      {2, strlen(albumNames[1]) + 1, albumNames[1]},
  };

  struct AlbumsListMessage toSerialize = {
      ALBUMS,   //
      0,        // value not important for calculation
      2,        //
      albumList //
  };

  serializeMessage(writeFd, (const struct Message *)&toSerialize);

  MessageSize calculatedSize = calculateMessageAlbumsListSize(&toSerialize);

  struct AlbumsListMessage *deserialized;
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

  char *songs[3] = {"Metallica - One", "Metallica - Two", "Metallica - Three"};
  struct SongListElement songList[3] = {
      {1, strlen(songs[0]) + 1, songs[0], 60},
      {2, strlen(songs[1]) + 1, songs[1], 120},
      {3, strlen(songs[2]) + 1, songs[2], 180},
  };

  struct SongsInAlbumMessage message = {
      SONGS_IN_ALBUM,
      0, // real size no needed for calculation
      3, songList};

  size_t expectedSize = sizeof(message.type) +         //
                        sizeof(message.size) +         //
                        sizeof(message.numberOfSongs); //

  for (size_t i = 0; i < 3; i++) {
    struct SongListElement song = songList[i];
    expectedSize += sizeof(song.songId);
    expectedSize += sizeof(song.nameLength);
    expectedSize += sizeof(char) * song.nameLength;
    expectedSize += sizeof(song.lengthInSeconds);
  }

  MessageSize calculatedSize = calculateMessageSongsInAlbumSize(&message);
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

static void test__SongsInAlbumMessageSerializationDeserialization(int writeFd,
                                                                  int readFd) {

  char *songs[3] = {"Metallica - One",    //
                    "Metallica - Two",    //
                    "Metallica - Three"}; //

  struct SongListElement songList[3] = {
      {1, strlen(songs[0]) + 1, songs[0], 60},
      {2, strlen(songs[1]) + 1, songs[1], 120},
      {3, strlen(songs[2]) + 1, songs[2], 180},
  };

  struct SongsInAlbumMessage toSerialize = {
      SONGS_IN_ALBUM,
      0, // real size no needed for calculation
      3, songList};

  struct SongsInAlbumMessage *deserialized;
  serializeMessage(writeFd, (struct Message *)&toSerialize);
  deserializeMessage(readFd, (struct Message **)&deserialized);

  assert(toSerialize.type == deserialized->type);
  assert(calculateMessageSongsInAlbumSize(&toSerialize) == deserialized->size);
  assert(toSerialize.numberOfSongs == deserialized->numberOfSongs);
}

int main() {

  int writeFd = creat("build/serialized.bin", S_IWUSR | S_IRUSR);
  int readFd = open("build/serialized.bin", S_IRUSR);

  test__DoListSongsInAlbumMessage__SerializationDeseralization(writeFd, readFd);
  test__DoListAlbumsListMessage__SerializationDeseralization(writeFd, readFd);
  test__AlbumsListMessageSizeCalculation();
  test__AlbumsListMessageSerializationDeserialization(writeFd, readFd);
  test__SongsInAlbumMessageSizeCalculation();
  test__SongsInAlbumMessageSerializationDeserialization(writeFd, readFd);
  test__SongMetadataMessageSizeCalculation();
  test__SongAudioDataDataMessageSizeCalculation();
  printf("Messages: all test passed\n");

  int forBreakpoint = 0;
}
