#include "library.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "logs.h"
#include <stdlib.h>

struct Library * library_init(struct Library * library) {
  library->library_path = (char *) MUSIC_LIBRARY_PATH;
  DIR * music_dir = opendir(MUSIC_LIBRARY_PATH);
  size_t music_dir_len = strlen(MUSIC_LIBRARY_PATH);
  char path_buffer[3*265];

  memcpy(path_buffer, MUSIC_LIBRARY_PATH, music_dir_len);
  path_buffer[music_dir_len] = '/';

  if(music_dir == NULL) {
    print_error("Error on opening music library dir: %s\n", strerror(errno));
  }

  struct LibraryAlbums * album_list = malloc(sizeof *album_list);
  struct LibrarySongs * song_list = malloc(sizeof *song_list);
  album_list->size = 10;
  album_list->items = malloc(10 * sizeof *album_list->items);
  song_list->size = 20;
  song_list->items = malloc(20 * sizeof *song_list->items);

  struct dirent * library_dirent;
  struct dirent * album_dirent;
  errno = 0;
  size_t album_count = 0;
  size_t song_count = 0;
  while((library_dirent = readdir(music_dir))) {

    if(library_dirent->d_name[0] == '.') {
      continue;
    }

    album_list->items[album_count].first_song_id = song_count;

    int album_name_len = strlen(library_dirent->d_name);

    if(album_count != 0 && album_list->size == album_count) {
      album_list->size <<= 2;
      album_list->items =
        realloc(album_list->items, album_list->size * sizeof *album_list->items);
    }

    memcpy(path_buffer + music_dir_len + 1, library_dirent->d_name, 256);
    DIR * album_dir = opendir(path_buffer);
    path_buffer[music_dir_len + 1 + album_name_len] = '/';

    while((album_dirent = readdir(album_dir))) {
      int song_name_len = strlen(album_dirent->d_name);
      if(strncmp(album_dirent->d_name + song_name_len - 4, "flac", 4)) {
        continue;
      }

      if(song_count != 0 && song_list->size == song_count) {
        song_list->size <<= 2;
        song_list->items =
          realloc(song_list->items, song_list->size * sizeof *song_list->items);
      }

      song_list->items[song_count].album_id = album_count;
      memcpy(song_list->items[song_count].name, album_dirent->d_name, 256);

      song_count++;
    }

    album_list->items[album_count].album_size
      = song_count - album_list->items[album_count].first_song_id;

    if(!album_list->items[album_count].album_size) {
      continue;
    }

    memcpy(album_list->items[album_count].name, library_dirent->d_name, 256);

    if(album_dir == NULL) {
      print_error("Error on opening album dir: %s\n", strerror(errno));
    }

    closedir(album_dir);
    album_count++;

  }

  album_list->size = album_count;
  album_list->items = realloc(album_list->items,
                              album_count* sizeof *album_list->items);

  song_list->size = song_count;
  song_list->items = realloc(song_list->items,
                              song_count* sizeof *song_list->items);

  if(errno != 0) {
    print_error("Error while iterating directory: %s\n", strerror(errno));
  }

  library->album_list = album_list;
  library->song_list = song_list;

  return library;
}


struct LibraryAlbums * library_albums(struct Library * library) {
  return library->album_list;
}

struct LibrarySongs * library_album_songs(struct Library * library, size_t album_id) {
  struct LibraryAlbumEntry * album = library->album_list->items + album_id;

  struct LibrarySongs * songs_in_album = malloc(sizeof *songs_in_album);
  songs_in_album->size = album->album_size;
  songs_in_album->items = library->song_list->items + album->first_song_id;
  songs_in_album->first_song_id = album->first_song_id;
  return songs_in_album;
}

char * library_song_build_path(struct Library * library, size_t song_id) {

  struct LibrarySongEntry * song = (library->song_list->items + song_id);
  struct LibraryAlbumEntry * album = (library->album_list->items + song->album_id);
  char * library_path = library->library_path;

  int library_path_size = strlen(library_path);
  int album_name_size = strlen(album->name);
  int song_name_size = strlen(song->name) + 1; // \0

  char * absolute_song_path = malloc(library_path_size
                                 + 2 /* 2x '/' */
                                 + album_name_size
                                 + song_name_size);

  memcpy(absolute_song_path, library_path, library_path_size);
  absolute_song_path[library_path_size] = '/';

  memcpy(absolute_song_path + library_path_size + 1, album->name, album_name_size);
  absolute_song_path[library_path_size + 1 + album_name_size] = '/';

  memcpy(absolute_song_path + library_path_size + 1 + album_name_size + 1,
         song->name, song_name_size);

  return absolute_song_path;
}
