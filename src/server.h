#ifndef FFUN_SERVER_H_
#define FFUN_SERVER_H_

#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>

enum { FFUN_SERVER_DEFAULT_PORT = 8080 };

enum { FFUN_SERVER_DEFAULT_CONNECTION_POOL = 5 };

const char FFUN_SERVER_DEFAULT_IP[] = "0.0.0.0";
const char FFUN_SERVER_MESSAGE_RECEIVED[] = "Server received: %s";

struct ServerContext {
  int socket;
  FILE * openedFile;
};

struct ClientContext {
  struct sockaddr * clientAddr;
  socklen_t clientAddrSize;
};

struct HandleClientArgs {
  uint16_t rawMessageSize;
  char * rawMessage;
  struct ServerContext * serverContext;
  struct ClientContext * clientContext;
};

void *handleClient(struct HandleClientArgs * args);

#endif // FFUN_SERVER_H_
