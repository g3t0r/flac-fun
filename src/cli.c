#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "logs.h"
#include "player_client.h"

static const char COMMAND_PLAY[] = "play";
static const char COMMAND_PAUSE[] = "pause";
static const char COMMAND_RESUME[] = "resume";
static const char COMMAND_STOP[] = "stop";

int main(int argc, char** argv) {

  struct PlayerClient player_client;

  player_client_connect_to_daemon(&player_client);

  if(!strncmp(*(argv+1), COMMAND_PLAY, sizeof(COMMAND_PLAY))) {

    if(argc < 3) {
      print_error("Missing song id\n");
      exit(1);
    }
    int song_id = atoi(*(argv+2));
    player_client_play(&player_client, song_id);

  } else if(!strncmp(*(argv+1), COMMAND_PAUSE, sizeof(COMMAND_PAUSE))) {

    print_debug("Received pause command\n");
    player_client_pause(&player_client);

  } else if(!strncmp(*(argv+1), COMMAND_RESUME, sizeof(COMMAND_RESUME))) {

    print_debug("Received resume command\n");
    player_client_resume(&player_client);

  } else if(!strncmp(*(argv+1), COMMAND_STOP, sizeof(COMMAND_PAUSE))) {

    print_debug("Received stop command\n");
    player_client_stop(&player_client);

  } else {
    assert(0 && "Unsupported command");
  }

  player_client_disconnect_from_player_daemon(&player_client);
} 
