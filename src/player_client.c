#include <assert.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>


#include "player_client.h"
#include "logs.h"
#include "config.h"
#include "messages.h"

int player_client_connect_to_daemon(struct PlayerClient * player_client) {
  player_client->conn_info.socket = socket(AF_INET, SOCK_STREAM, 0);
  if(player_client->conn_info.socket == -1) {
    print_error("Problem while creating socket: %s\n",
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

    print_error("Problem while binding socket: %s\n",
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

    print_error("Problem while connecting to player_daemon: %s\n",
        strerror(errno));
    exit(1);
  }

  player_client->player_daemon.connected = 1;

  return 0;
}


int player_client_connect_to_content_server(struct PlayerClient * player_client) {

  player_client->conn_info.socket = socket(AF_INET, SOCK_STREAM, 0);
  if(player_client->conn_info.socket == -1) {
    print_error("Problem while creating socket: %s\n",
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

    print_error("Problem while binding socket: %s\n",
        strerror(errno));
    exit(1);
  }

  player_client->content_server.sock_addr.sin_family = AF_INET;
  player_client->content_server.sock_addr.sin_port = htons(FFUN_CONTENT_SERVER_PORT_TCP);
  inet_aton(FFUN_CONTENT_SERVER_IP,
      &player_client->content_server.sock_addr.sin_addr);


  if(connect(
        player_client->conn_info.socket,
        (struct sockaddr *) &player_client->content_server.sock_addr,
        sizeof(struct sockaddr_in))) {

    print_error("Problem while connecting to content_server: %s\n",
        strerror(errno));
    exit(1);
  }

  player_client->content_server.connected = 1;

  return 0;
}

int player_client_play(struct PlayerClient * player_client, int song_id) {

  struct MessageHeader header = {
  0,
  sizeof(struct MessageHeader)
  + sizeof(struct PlaySongMessage),
  MESSAGE_TYPE_PLAY_SONG
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
    sizeof(struct MessageHeader),
    MESSAGE_TYPE_PAUSE
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
    sizeof(struct MessageHeader),
    MESSAGE_TYPE_RESUME
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

int player_client_stop(struct PlayerClient * player_client) {

  struct MessageHeader header = {
    0,
    sizeof(struct MessageHeader),
    MESSAGE_TYPE_STOP
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



struct AlbumListMessage *
player_client_list_albums(struct PlayerClient * player_client) {

  struct MessageHeader req_header;
  struct MessageHeader resp_header;
  struct AlbumListMessage * msg_album_list = NULL;
  req_header.seq = 0;
  req_header.size = MSG_HEADER_SIZE;
  req_header.type = MESSAGE_TYPE_ALBUM_LIST_REQ;
  char * buffer = malloc(req_header.size);
  messages_header_serialize(&req_header, buffer);

  size_t total_bytes = req_header.size;
  size_t transfered_bytes = 0;

  struct pollfd read_poll_fd;
  read_poll_fd.fd = player_client->conn_info.socket;
  read_poll_fd.events = POLLIN;

  send(player_client->conn_info.socket, buffer, total_bytes,0);

  int poll_result = poll(&read_poll_fd, 1, -1);
  if(poll_result == 1) {
    if(read_poll_fd.revents & POLLIN) {
      msg_album_list = malloc(sizeof *msg_album_list);
      transfered_bytes = recv(read_poll_fd.fd, buffer, MSG_HEADER_SIZE, 0);
      messages_header_deserialize(buffer, &resp_header);

      buffer = realloc(buffer, resp_header.size - MSG_HEADER_SIZE);
      transfered_bytes = recv(read_poll_fd.fd, buffer, resp_header.size - MSG_HEADER_SIZE, 0);
      messages_album_list_resp_msg_deserialize(buffer, msg_album_list);
    }
  } else {
    print_error("Error on poll\n");
  }
  return msg_album_list;
}

struct AlbumSongsRespMessage *
player_client_show_album(struct PlayerClient * player_client, uint32_t album_id) {

  struct {
    struct MessageHeader header;
    struct AlbumSongsReqMessage message;
  } request;

  struct {
    struct MessageHeader header;
    struct AlbumSongsRespMessage * message;
  } response;

  request.header.seq = 0;
  request.header.size = MSG_HEADER_SIZE + MSG_ALBUM_SONGS_REQ_SIZE;
  request.header.type = MESSAGE_TYPE_ALBUM_SONGS_REQ;
  request.message.album_id = album_id;
  char * buffer = malloc(MSG_HEADER_SIZE + MSG_ALBUM_SONGS_REQ_SIZE);
  int byte_shift = messages_header_serialize(&request.header, buffer);
  messages_album_songs_req_msg_serialize(&request.message, buffer + byte_shift);

  size_t send_result = send(player_client->conn_info.socket, buffer, MSG_HEADER_SIZE + MSG_ALBUM_SONGS_REQ_SIZE, 0);
  assert(send_result == MSG_HEADER_SIZE + MSG_ALBUM_SONGS_REQ_SIZE);

  struct pollfd read_poll_fd;
  read_poll_fd.fd = player_client->conn_info.socket;
  read_poll_fd.events = POLLIN;

  int poll_result = poll(&read_poll_fd, 1, -1);
  if(poll_result != 1) {
    print_error("Problem with poll: %s\n", strerror(errno));
    return NULL;
  }

  if(!(read_poll_fd.revents & POLLIN)) {
    print_error("Not a pollin event: %s\n", strerror(errno));
    return NULL;
  }

  recv(read_poll_fd.fd, buffer, MSG_HEADER_SIZE, 0);
  messages_header_deserialize(buffer, &response.header);
  buffer = realloc(buffer, response.header.size - MSG_HEADER_SIZE);
  recv(read_poll_fd.fd, buffer, response.header.size - MSG_HEADER_SIZE, 0);
  response.message = malloc(sizeof *response.message);
  messages_album_songs_resp_deserialize(buffer, response.message);

  return response.message;
}


void player_client_disconnect_from_player_daemon(struct PlayerClient * player_client) {
  player_client->player_daemon.connected = 0;
  close(player_client->conn_info.socket);
}

void player_client_disconnect_from_content_server(struct PlayerClient * player_client) {
  player_client->player_daemon.connected = 0;
  close(player_client->conn_info.socket);
}
