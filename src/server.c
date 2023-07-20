#include "server.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <bits/pthreadtypes.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>

int main(int argc, char **argv) {

  int sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sd == -1) {
    printf("Problem creating socket: %s\n", strerror(errno));
    exit(1);
  }

  int optValue = 1;
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optValue, sizeof(int));

  struct sockaddr_in addrIn;
  addrIn.sin_family = AF_INET;
  addrIn.sin_port = htons(8080);
  inet_pton(AF_INET, "0.0.0.0",
            &addrIn.sin_addr.s_addr);
  addrIn.sin_addr.s_addr = htonl(addrIn.sin_addr.s_addr);

  int result = bind(sd, (const struct sockaddr *)&addrIn, sizeof(addrIn));
  if (result != 0) {
    printf("Problem binding socket: %s\n", strerror(errno));
    exit(1);
  }


  struct pollfd pollFileDescriptor;
  pollFileDescriptor.fd = sd;
  pollFileDescriptor.events = POLLIN;
  nfds_t nfds = 1;

  while (1) {
    printf("Waiting for poll\n");
    int result = poll(&pollFileDescriptor, nfds, -1);
    printf("Results: %d, R-events: %d\n", result, pollFileDescriptor.revents);

    if(result == -1) {
      printf("Error with poll\n");
      close(sd);
      pthread_exit(0);
    }

    if(pollFileDescriptor.revents & POLLNVAL) {
      printf("Incorrect poll request\n");
    } else if(pollFileDescriptor.revents & POLLERR) {
      printf("Socket hung up\n");
    }

    if(result > 0 && (pollFileDescriptor.revents & POLLIN) != 0) {
      char buffer[256];
      memset(buffer, 0, 256);
      int readBytes = read(pollFileDescriptor.fd, (void*) buffer, 256);

      if(readBytes == 0) {
        break;
      }

      printf("%s\n", buffer);
    }

  }

}

void * handleConnection(struct ConnectionContext *context) {
  printf("Hello from thread tid=%uf\n", (unsigned int) pthread_self());

  struct pollfd pollFileDescriptor;
  pollFileDescriptor.fd = context->connectionSockFd;
  pollFileDescriptor.events = POLLIN;

  nfds_t nfds = 1;


  printf("ConnectionSd inside handleConnection: %x\n", context->connectionSockFd);
  while(1) {
    int result = poll(&pollFileDescriptor, nfds, -1);
    printf("Results: %d, R-events: %d\n", result, pollFileDescriptor.revents);

    if(result == -1) {
      printf("Error with poll\n");
      close(context->connectionSockFd);
      pthread_exit(0);
    }

    if(pollFileDescriptor.revents & POLLNVAL) {
      printf("Incorrect poll request\n");
    } else if(pollFileDescriptor.revents & POLLERR) {
      printf("Socket hung up\n");
    }

    if(result > 0 && (pollFileDescriptor.revents & POLLIN) != 0) {
      char buffer[256];
      int readBytes = read(pollFileDescriptor.fd, (void*) buffer, 256);

      if(readBytes == 0) {
        break;
      }

      char bufferOut[512];
      snprintf(bufferOut, 512, FFUN_SERVER_MESSAGE_RECEIVED, buffer);
      write(pollFileDescriptor.fd, bufferOut, strlen(bufferOut));
    }
  }

  printf("Client disconnected\n");


  return 0;
}
