#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "logs.h"
#include "player_client.h"

enum Command {
CMD_PLAY = 0,
CMD_PAUSE,
CMD_RESUME,
CMD_STOP,
CMD_LIST_ALBUMS,
CMD_SHOW_ALBUM,
CMD_INVALID
};

static char* commands_list[] = {
"play",
"pause",
"resume",
"stop",
"albums",
"album"
};

enum Command parse_command(char * command) {
  size_t command_size;

  if((command_size = strlen(command)) == 0) {
    return CMD_INVALID;
  }

  for(int i = 0; i < sizeof(commands_list) / sizeof(char *); i++) {
    if(!strncmp(command, commands_list[i], command_size)) {
      return i;
    }
  }

  return CMD_INVALID;
}

int main(int argc, char** argv) {

  struct PlayerClient player_client;

  player_client_connect_to_daemon(&player_client);

  enum Command parsed_command = parse_command(*(argv+1));

  switch(parsed_command) {                      \

    case CMD_INVALID: {
      print_error("Invalid command\n");
      break;
    }

    case CMD_PLAY: {

      if(argc < 3) {
        print_error("Missing song id\n");
        exit(1);
      }
      int song_id = atoi(*(argv+2));
      player_client_play(&player_client, song_id);
      break;

    }

    case CMD_PAUSE: {

      print_debug("Received pause command\n");
      player_client_pause(&player_client);
      break;

    }

    case CMD_RESUME: {

      print_debug("Received resume command\n");
      player_client_resume(&player_client);
      break;

    }

    case CMD_STOP: {

      print_debug("Received stop command\n");
      player_client_stop(&player_client);
      break;

    }

    default: {
      print_error("Not supported yet\n");
    }

  }

  player_client_disconnect_from_player_daemon(&player_client);
} 
