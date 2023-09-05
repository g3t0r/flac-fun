#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "player_client.h"
#include "logs.h"
#include "config.h"
#include "messages.h"

int player_client_connect_to_daemon(struct PlayerClient * player_client) {
  player_client->conn_info.socket = socket(AF_INET, SOCK_STREAM, 0);
  if(player_client->conn_info.socket == -1) {
    printError("Problem while creating socket: %s\n",
        strerror(errno));
    exit(1);
  }


  player_client->conn_info.sock_addr.sin_family = AF_INET;
  player_client->conn_info.sock_addr.sin_port = 0;
  inet_aton("0.0.0.0",
      &player_client->conn_info.sock_addr.sin_addr);

  if(bind(player_client->conn_info.socket,
        (struct sockaddr *) &player_client->conn_info.sock_addr,
        sizeof(struct sockaddr_in))) {

    printError("Problem while binding socket: %s\n",
        strerror(errno));
    exit(1);
  }

  player_client->player_daemon.sock_addr.sin_family = AF_INET;
  player_client->player_daemon.sock_addr.sin_port = htons(FFUN_PLAYER_DAEMON_PORT_TCP);
  inet_aton("127.0.0.1",
      &player_client->player_daemon.sock_addr.sin_addr);


  if(connect(
        player_client->conn_info.socket,
        (struct sockaddr *) &player_client->player_daemon.sock_addr,
        sizeof(struct sockaddr_in))) {

    printError("Problem while connecting to player_daemon: %s\n",
        strerror(errno));
    exit(1);
  }

  player_client->player_daemon.connected = 1;

  return 0;
}

int player_client_play(struct PlayerClient * player_client, int song_id) {

  struct MessageHeader header = {
    0,
    MESSAGE_TYPE_PLAY_SONG,
    sizeof(struct MessageHeader)
  };

  struct PlaySongMessage message;

  char udp_data[FFUN_UDP_DGRAM_MAX_SIZE];

  int udp_data_size = messages_header_serialize(&header, udp_data);
  udp_data_size += messages_play_song_msg_serialize(&message,
      udp_data + udp_data_size);

  send(
      player_client->conn_info.socket,
      udp_data,
      udp_data_size,
      0);

  return 0;
}

int player_client_pause(struct PlayerClient * player_client) {

  struct MessageHeader header = {
    0,
    MESSAGE_TYPE_PAUSE,
    sizeof(struct MessageHeader)
  };

  char udp_data[FFUN_UDP_DGRAM_MAX_SIZE];

  int udp_data_size =  messages_header_serialize(&header, udp_data);


  send(
      player_client->conn_info.socket,
      udp_data,
      udp_data_size,
      0);

  return 0;
}

int player_client_resume(struct PlayerClient * player_client) {

  struct MessageHeader header = {
    0,
    MESSAGE_TYPE_RESUME,
    sizeof(struct MessageHeader)
  };

  char udp_data[FFUN_UDP_DGRAM_MAX_SIZE];

  int udp_data_size =  messages_header_serialize(&header, udp_data);


  send(
      player_client->conn_info.socket,
      udp_data,
      udp_data_size,
      0);

  return 0;
}


void player_client_disconnect_from_player_daemon(struct PlayerClient * player_client) {
  close(player_client->conn_info.socket);
}
