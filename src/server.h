#ifndef FFUN_SERVER_H_
#define FFUN_SERVER_H_

#include <netinet/in.h>

enum { FFUN_SERVER_DEFAULT_PORT = 1144 };

enum { FFUN_SERVER_DEFAULT_CONNECTION_POOL = 5 };

const char FFUN_SERVER_DEFAULT_IP[] = "0.0.0.0";
const char FFUN_SERVER_MESSAGE_RECEIVED[] = "Server received: %s";

struct ConnectionContext {
  int connectionSockFd;
  struct sockaddr_in *clientAddrIn;
};

void *handleConnection(struct ConnectionContext *context);

#endif // FFUN_SERVER_H_
