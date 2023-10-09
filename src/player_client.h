#ifndef PLAYER_CLIENT_H_
#define PLAYER_CLIENT_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include "messages.h"

struct PlayerClient {

  struct {
    int socket;
    struct sockaddr_in sock_addr;  
  } conn_info;

  struct {
    int connected;
    struct sockaddr_in sock_addr;
  } player_daemon;

  struct {
    int connected;
    struct sockaddr_in sock_addr;
  } content_server;

};

int player_client_connect_to_daemon(struct PlayerClient * player_client);
int player_client_connect_to_content_server(struct PlayerClient * player_client);

int player_client_play(struct PlayerClient * player_client, int song_id);
int player_client_pause(struct PlayerClient * player_client);
int player_client_resume(struct PlayerClient * player_client);
int player_client_stop(struct PlayerClient * player_client);

struct AlbumListMessage *
player_client_list_albums(struct PlayerClient * player_client);

struct AlbumSongsRespMessage *
player_client_show_album(struct PlayerClient * player_client, uint32_t album_id);

void player_client_disconnect_from_player_daemon(struct PlayerClient * player_client);
void player_client_disconnect_from_content_server(struct PlayerClient * player_client);

#endif
