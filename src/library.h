#ifndef __FFUN_LIBRARY__
#define __FFUN_LIBRARY__

#include "config.h"
#include <stdint.h>
#include <stddef.h>

struct Library {
  char * library_path;
  struct LibraryAlbums * album_list;
  struct LibrarySongs * song_list;
};

struct LibraryAlbumEntry {
  size_t first_song_id;
  int album_size;
  char name[256];
};

struct LibraryAlbums {
  size_t size;
  struct LibraryAlbumEntry * items;
};

struct LibrarySongEntry {
  size_t album_id;
  char name[256];
};

struct LibrarySongs {
  uint32_t first_song_id;
  size_t size;
  struct LibrarySongEntry * items;
};

struct Library * library_init(struct Library * library);

struct LibraryAlbums * library_albums(struct Library * library);

struct LibrarySongs * library_album_songs(struct Library * library, size_t album_id);

char * library_song_build_path(struct Library * library, size_t song_id);

#endif
