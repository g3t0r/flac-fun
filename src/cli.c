#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "logs.h"
#include "player_client.h"

static const char COMMAND_PLAY[] = "play";
static const char COMMAND_PAUSE[] = "pause";
static const char COMMAND_RESUME[] = "resume";

int main(int argc, char** argv) {

  struct PlayerClient player_client;

  player_client_connect_to_daemon(&player_client);

  if(!strncmp(*(argv+1), COMMAND_PLAY, sizeof(COMMAND_PLAY))) {

    if(argc < 3) {
      printError("Missing song id\n");
      exit(1);
    }
    int song_id = atoi(*(argv+2));
    player_client_play(&player_client, song_id);

  } else if(!strncmp(*(argv+1), COMMAND_PAUSE, sizeof(COMMAND_PAUSE))) {

    player_client_pause(&player_client);

  } else if(!strncmp(*(argv+1), COMMAND_RESUME, sizeof(COMMAND_RESUME))) {

    player_client_resume(&player_client);

  }

  player_client_disconnect_from_player_daemon(&player_client);
} 
