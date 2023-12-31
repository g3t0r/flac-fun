#ifndef FFUN_SERVER_H_
#define FFUN_SERVER_H_

#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include "messages.h"
#include "library.h"

enum { FFUN_SERVER_DEFAULT_PORT = 8080 };

enum { FFUN_SERVER_DEFAULT_CONNECTION_POOL = 5 };

const char FFUN_SERVER_DEFAULT_IP[] = "0.0.0.0";
const char FFUN_SERVER_MESSAGE_RECEIVED[] = "Server received: %s";

struct Server {

  // right now only single client support
  FILE * opened_file;
  size_t current_song_id;

  struct Library library;

  struct {
    int socket;
    struct sockaddr_in addr;
  } udp_info;

  struct {
    int socket;
    struct sockaddr_in addr;
  } tcp_info;

};

struct HandleClientArgs {
  uint16_t rawMessageSize;
  char * rawMessage;
  struct ServerContext * serverContext;
  struct ClientContext * clientContext;
};

struct HandleFeedMeMsgFuncArgs {
  struct Server * server;
  struct sockaddr_in client_sockaddr;
  socklen_t client_sockaddr_size;
  struct FeedMeMessage message;
};

struct HandleTcpClientConnArgs {
  int client_socket;
  struct Server * server;
  struct sockaddr_in client_sockaddr;
  socklen_t client_sockaddr_size;
};


#endif // FFUN_SERVER_H_
