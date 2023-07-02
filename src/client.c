#include "client.h"

#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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
  clientAddrIn.sin_port = htons(FFUN_CLIENT_DEFAULT_PORT);
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
  serverAddrIn.sin_port = htons(1144);
  inet_pton(AF_INET, "127.0.0.1", (void *)&serverAddrIn.sin_addr.s_addr);

  result =
      connect(sd, (const struct sockaddr *)&serverAddrIn, sizeof(serverAddrIn));

  if (result != 0) {
    printf("Problem on connecting to server: %s\n", strerror(errno));
    exit(1);
  }

  enum { WRITE_FD = 0, READ_FD = 1 };

  struct pollfd pollFileDescriptors[2];
  pollFileDescriptors[WRITE_FD].fd = sd;
  pollFileDescriptors[WRITE_FD].events = POLLOUT;
  pollFileDescriptors[READ_FD].fd = sd;
  pollFileDescriptors[READ_FD].events = POLLIN;

  char buffer[1024];

  while (1) {
    result = poll(pollFileDescriptors, 2, -1);
    if (result == -1) {
      printf("Failed to poll\n");
      exit(1);
    }

    if (pollFileDescriptors[WRITE_FD].revents | POLLOUT) {
      sprintf((char *)&buffer, "Hello, I'm a client");

      int writeBytes = write(sd, buffer, strlen(buffer) + 1);

      sleep(5);
    }

    if(pollFileDescriptors[READ_FD].revents | POLLIN) {
        int readBytes = read(sd, buffer, sizeof(buffer));
        if(readBytes == -1) {
            printf("Error reading from server\n");
            exit(1);
        } else if(readBytes == 0) {
            printf("Server dropped connection\n");
            exit(1);
        }

        printf("Received message from server: %s\n", buffer);
    }
  }
}
