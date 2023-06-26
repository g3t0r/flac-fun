#ifndef MUSICLIBRARY_H_
#define MUSICLIBRARY_H_

#define FFUN_SONG_NAME_LENGTH 50
#define FFUN_ALBUM_NAME_LENGTH 50
#define FFUN_ARTIST_NAME_LENGTH 50

#include <stdint.h>

struct {
  char name[FFUN_SONG_NAME_LENGTH];
  char albumName[FFUN_ALBUM_NAME_LENGTH];
  char artist[FFUN_ARTIST_NAME_LENGTH];
} typedef FFUN__SongMetadata;

struct {
  char * fileName;
  FFUN__SongMetadata * metadata;
} typedef FFUN_Song;

#endif // MUSICLIBRARY_H_
