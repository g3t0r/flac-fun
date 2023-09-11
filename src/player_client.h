#ifndef PLAYER_CLIENT_H_
#define PLAYER_CLIENT_H_

#include <sys/socket.h>
#include <netinet/in.h>

struct PlayerClient {
  struct {
    int socket;
    struct sockaddr_in sock_addr;  
  } conn_info;
  struct {
    int connected;
    struct sockaddr_in sock_addr;
  } player_daemon;
};

int player_client_connect_to_daemon(struct PlayerClient * player_client);

int player_client_play(struct PlayerClient * player_client, int song_id);
int player_client_pause(struct PlayerClient * player_client);
int player_client_resume(struct PlayerClient * player_client);
int player_client_stop(struct PlayerClient * player_client);

void player_client_disconnect_from_player_daemon(struct PlayerClient * player_client);

#endif
