#ifndef __FFUN_LIBRARY__
#define __FFUN_LIBRARY__

#include "config.h"
#include <stdint.h>
#include <stddef.h>

struct Library {
  char * library_path;
  struct Albums * album_list;
  struct Songs * song_list;
};

struct AlbumEntry {
  size_t first_song_id;
  int album_size;
  char name[256];
};

struct Albums {
  size_t size;
  struct AlbumEntry * items;
};

struct SongEntry {
  size_t album_id;
  char name[256];
};

struct Songs {
  size_t size;
  struct SongEntry * items;
};

struct Library * library_init(struct Library * library);

struct Albums * library_albums(struct Library * library);

void library_album_songs(struct Library * library, size_t album_id);

char * library_song_build_path(struct Library * library, size_t song_id);

#endif
