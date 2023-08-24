#include "client.h"
#include "bytes.h"
#include "circle-buffer.h"
#include "messages.h"

#include "config.h"
#include "playback.h"
#include <ao/ao.h>
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <assert.h>
#include <bits/pthreadtypes.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
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

struct HandleDataMessageArgs {
  struct CircleBuffer *flacDataCircleBuffer;
  sem_t * bufferMutex;
  sem_t * bufferRead;
  sem_t * bufferWrite;
  struct ByteArray byteArray;
};

void *handleDataMessageFn(struct HandleDataMessageArgs *args) {

  struct MessageHeader header;
  struct DataMessage dataMessage;

  int headerSize = deserializeMessageHeader(args->byteArray.buffer, &header);
  assert(header.type == DATA);
  printf("Header seq: %d\n", header.seq);
  deserializeDataMessage(args->byteArray.buffer+headerSize, &dataMessage);

  char * data = malloc(dataMessage.dataSize);
  memcpy(data, dataMessage.data, dataMessage.dataSize);
  free(dataMessage.data);

  sem_wait(args->bufferWrite);
  sem_wait(args->bufferMutex);

  struct CircleBufferEntry * createdEntry =
    writeDataToBuffer(args->flacDataCircleBuffer, data, dataMessage.dataSize);
  assert(createdEntry != NULL);

  sem_post(args->bufferMutex);
  sem_post(args->bufferRead);

  return NULL;
}

int main(int argc, char **argv) {

  // begin server init
  struct ServerInfo serverInfo;
  serverInfo.socket = initializeSocket();
  // createServerInfo(&serverInfo, "192.168.0.175", 8080);
  createServerInfo(&serverInfo, "0.0.0.0", 8080);

  if (connect(serverInfo.socket, (struct sockaddr *)&serverInfo.addr,
              serverInfo.addrLen) != 0) {
    printf("Problem connecting to server, error %s\n", strerror(errno));
  }

  // begin playback init
  struct Playback *playback = malloc(sizeof(struct Playback));
  playback->socket = serverInfo.socket;
  initPlayback(playback);

  startPlayback(playback);
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

int global_datagram_seq = 0;
