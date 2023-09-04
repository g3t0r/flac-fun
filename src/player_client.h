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

#endif
