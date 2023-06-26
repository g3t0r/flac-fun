#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pathutils.h"

#define __STDC_FORMAT_MACROS

#define CHUNK_SIZE 1024





char * getLibraryPath() {
  char * home = getenv("HOME");
  char * library = "/Music/Bandcamp";
  char * fullPath = calloc(strlen(home) + strlen(library) + 1, sizeof(char));
  strncat(fullPath, home, strlen(home));
  strncat(fullPath, library, strlen(library));
  return fullPath;
}

int main(int argc, char **argv) {


  FILE * readFd = fopen("audio/1.flac", "rb");
  FILE * writeFD = fopen("audio/output.flac", "wb");

  char buffer[CHUNK_SIZE];

  int bytesRead;
  int i = 0;
  while((bytesRead = fread(&buffer, sizeof(char), CHUNK_SIZE, readFd)) != 0) {
    if(i == 1000) {
      i = 0;
      continue;
    }
    fwrite(&buffer, sizeof(char), bytesRead, writeFD);
    printf("After sleep, i = %d\n", i);
    i++;
  }

  fclose(readFd);
  fclose(writeFD);

  return 0;
}
