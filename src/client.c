#include "client.h"
#include "messages.h"

#include "config.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char **argv) {
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
  clientAddrIn.sin_family = AF_INET;
  clientAddrIn.sin_port = 0;
  inet_pton(AF_INET, (const char *)&FFUN_CLIENT_DEFAULT_IP,
            &clientAddrIn.sin_addr.s_addr);
  clientAddrIn.sin_addr.s_addr = htonl(clientAddrIn.sin_addr.s_addr);

  int result =
      bind(sd, (const struct sockaddr *)&clientAddrIn, sizeof(clientAddrIn));

  if (result != 0) {
    printf("Problem binding socket: %s\n", strerror(errno));
    exit(1);
  }

  struct sockaddr_in serverAddrIn;
  serverAddrIn.sin_family = AF_INET;
  serverAddrIn.sin_port = htons(8080);
  inet_pton(AF_INET, "127.0.0.1", (void *)&serverAddrIn.sin_addr.s_addr);

  if (connect(sd, (struct sockaddr *)&serverAddrIn, sizeof(serverAddrIn)) !=
      0) {
    printf("Problem connecting to server, error %s\n", strerror(errno));
  }

  struct MessageHeader header;
  header.size = sizeof(struct FeedMeMessage);
  header.type = FEED_ME;
  struct FeedMeMessage feedMe;
  feedMe.dataSize = 500;

  char buffer[FFUN_UDP_DGRAM_MAX_SIZE];

  uint16_t writtenBytes = serializeMessageHeader(&header, buffer);
  writtenBytes += serializeFeedMeMessage(&feedMe, buffer+writtenBytes);

  send(sd, buffer, writtenBytes, 0);
  printf("Message send, waiting for response\n");

  struct pollfd pfd;
  pfd.fd = sd;
  pfd.events = POLLIN;

  int pollResult =  poll(&pfd, 1, 0);
  if(pollResult == -1) {
    printf("Poll error: %s\n", strerror(errno));
  }

  recv(sd, buffer, FFUN_UDP_DGRAM_MAX_SIZE, 0);
  struct DataMessage message;
  int readBytes = deserializeDataMessage(buffer, &message);
  message.data[message.dataSize - 1] = '\0';
  printf("Received data: %s\n", message.data);

}
