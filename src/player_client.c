#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "player_client.h"
#include "logs.h"
#include "config.h"

int player_client_connect_to_daemon(struct PlayerClient * player_client) {
  player_client->conn_info.socket = socket(AF_INET, SOCK_STREAM, 0);
  if(player_client->conn_info.socket == -1) {
    printError("Problem while creating socket: %s\n",
        strerror(errno));
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
  }

  player_client->player_daemon.sock_addr.sin_family = AF_INET;
  player_client->player_daemon.sock_addr.sin_port = FFUN_PLAYER_DAEMON_PORT_UDP;
  inet_aton(FFUN_PLAYER_DAEMON_IP,
      &player_client->player_daemon.sock_addr.sin_addr);


  if(connect(
        player_client->conn_info.socket,
        (struct sockaddr *) &player_client->conn_info.sock_addr,
        sizeof(struct sockaddr_in))) {

    printError("Problem while connecting to player_daemon: %s\n",
        strerror(errno));
  }

}
int player_client_play(int songId);
int player_client_toggle_pause();
