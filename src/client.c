#include "client.h"
#include "messages.h"

#include "config.h"
#include "playback.h"
#include <ao/ao.h>
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

struct ServerInfo {
  int socket;
  struct sockaddr_in addr;
  socklen_t addrLen;
};

static int initializeSocket();
static void createServerInfo(struct ServerInfo *dst, const char *ipv4,
                             uint16_t port);
struct RequestDataArgs {
  int socket;
  struct Playback *playback;
};

void requestData(struct RequestDataArgs *args, char **data, size_t * dataSize);

int main(int argc, char **argv) {

  ao_initialize();
  struct ServerInfo serverInfo;
  serverInfo.socket = initializeSocket();
  createServerInfo(&serverInfo, "127.0.0.1", 8080);

  if (connect(serverInfo.socket, (struct sockaddr *)&serverInfo.addr,
              serverInfo.addrLen) != 0) {
    printf("Problem connecting to server, error %s\n", strerror(errno));
  }

  struct Playback *playback = malloc(sizeof(struct Playback));
  struct RequestDataArgs requestDataArgs;
  requestDataArgs.socket = serverInfo.socket;
  requestDataArgs.playback = playback;
  playback->feedMeCb = (void (*)(void *, char **, size_t *))requestData;
  playback->args = &requestDataArgs;
  initPlayback(playback);
  play(playback);
}

static int initializeSocket() {

  int sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sd == -1) {
    printf("Problem creating socket: %s\n", strerror(errno));
    exit(1);
  }

  {
    int optValue = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optValue, sizeof(int));
  }

  struct sockaddr_in clientAddrIn;
  inet_pton(AF_INET, (const char *)&FFUN_CLIENT_DEFAULT_IP,
            &clientAddrIn.sin_addr.s_addr);

  clientAddrIn.sin_family = AF_INET;
  clientAddrIn.sin_port = 0;
  clientAddrIn.sin_addr.s_addr = htonl(clientAddrIn.sin_addr.s_addr);

  int result =
      bind(sd, (const struct sockaddr *)&clientAddrIn, sizeof(clientAddrIn));

  if (result != 0) {
    printf("Problem binding socket: %s\n", strerror(errno));
    exit(1);
  }
  return sd;
}

static void createServerInfo(struct ServerInfo *dst, const char *ipv4,
                             uint16_t port) {
  dst->addrLen = sizeof(struct sockaddr_in);
  dst->addr.sin_family = AF_INET;
  dst->addr.sin_port = htons(port);
  inet_pton(AF_INET, ipv4, (void *)&dst->addr.sin_addr.s_addr);
}

void requestData(struct RequestDataArgs *args, char **data, size_t * dataSize) {
  struct MessageHeader header;
  header.size = sizeof(struct FeedMeMessage);
  header.type = FEED_ME;
  struct FeedMeMessage feedMe;
  feedMe.dataSize = 500;

  char buffer[FFUN_UDP_DGRAM_MAX_SIZE];

  struct pollfd pfd;
  pfd.fd = args->socket;
  pfd.events = POLLIN;

  header.size = sizeof(struct FeedMeMessage);
  header.type = FEED_ME;
  uint16_t writtenBytes = serializeMessageHeader(&header, buffer);
  writtenBytes += serializeFeedMeMessage(&feedMe, buffer + writtenBytes);

  send(args->socket, buffer, writtenBytes, 0);
  //printf("Message send, waiting for response\n");

  int pollResult = poll(&pfd, 1, 0);
  if (pollResult == -1) {
    printf("Poll error: %s\n", strerror(errno));
  }

  recv(args->socket, buffer, FFUN_UDP_DGRAM_MAX_SIZE, 0);
  struct DataMessage message;
  int readBytes = deserializeMessageHeader(buffer, &header);
  readBytes += deserializeDataMessage(buffer + readBytes, &message);
  *data = malloc(sizeof(char) * readBytes);
  memcpy(*data, buffer, readBytes);
  *dataSize = readBytes;
  //printf("Received data: %s\n", message.data);
}
