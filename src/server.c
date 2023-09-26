#include "server.h"
#include "config.h"
#include "logs.h"
#include "messages.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <assert.h>
#include <bits/pthreadtypes.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

struct {
  char music_dir_path[1024];
  size_t number_of_albums;
} MusicCollectionInfo;

struct {
  size_t album_id;
  size_t number_of_songs;
} AlbumInfo;

const char MUSIC_LIBRARY_PATH[] = "/tmp/music";

void initializeLibrary();

static void initializeServer(struct ServerContext *serverContext);

static int handleFeedMeMessage(struct ServerContext *serverContext,
                               struct ClientContext *clientContext,
                               struct MessageHeader *header,
                               struct FeedMeMessage *message);
FILE *debugFile;

int main(int argc, char **argv) {

  initializeLibrary();
  return 0;
  struct ServerContext serverContext;
  initializeServer(&serverContext);
  printf("sd: %u\n", serverContext.socket);
  debugFile = fopen("./audio/server.data.bin", "wb");

  while (1) {
    printf("Inside loop\n");
    struct sockaddr_in *connAddrIn = malloc(sizeof(struct sockaddr_in));
    socklen_t connSockAddrLen;

    struct pollfd pollFileDescriptor;
    pollFileDescriptor.fd = serverContext.socket;
    pollFileDescriptor.events = POLLIN;

    nfds_t nfds = 1;

    int result = poll(&pollFileDescriptor, nfds, -1);
    printf("Results: %d, R-events: %d\n", result, pollFileDescriptor.revents);

    if (result == -1) {
      printf("Error with poll\n");
      close(serverContext.socket);
    }

    if (pollFileDescriptor.revents & POLLNVAL) {
      printf("Incorrect poll request\n");
    } else if (pollFileDescriptor.revents & POLLERR) {
      printf("Socket hung up\n");
    }

    if (result > 0 && (pollFileDescriptor.revents & POLLIN) != 0) {
      struct ClientContext *clientContext =
          malloc(sizeof(struct ClientContext));

      clientContext->clientAddrSize = sizeof(struct sockaddr_in);

      char *buffer = malloc(sizeof(char) * FFUN_UDP_DGRAM_MAX_SIZE);
      int readBytes = recvfrom(pollFileDescriptor.fd, (void *)buffer,
                               FFUN_UDP_DGRAM_MAX_SIZE, 0,
                               (struct sockaddr *)&clientContext->clientAddr,
                               &clientContext->clientAddrSize);

      printf("ReadBytes: %u\n", readBytes);

      if (readBytes == 0) {
        break;
      }

      struct HandleClientArgs *handleClientArgs =
          malloc(sizeof(struct HandleClientArgs));

      handleClientArgs->rawMessageSize = readBytes;
      handleClientArgs->rawMessage = buffer;

      handleClientArgs->clientContext = clientContext;
      handleClientArgs->serverContext = &serverContext;

      pthread_t thread;
      pthread_create(&thread, NULL, (void *(*)(void *))handleClient,
                     handleClientArgs);
    }
  }
}


void initializeServer(struct ServerContext *serverContext) {

  int sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sd == -1) {
    printf("Problem creating socket: %s\n", strerror(errno));
    exit(1);
  }

  int optValue = 1;
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optValue, sizeof(int));

  struct sockaddr_in addrIn;
  addrIn.sin_family = AF_INET;
  addrIn.sin_port = htons(FFUN_SERVER_DEFAULT_PORT);
  inet_pton(AF_INET, (const char *)&FFUN_SERVER_DEFAULT_IP,
            &addrIn.sin_addr.s_addr);
  addrIn.sin_addr.s_addr = htonl(addrIn.sin_addr.s_addr);

  int result = bind(sd, (const struct sockaddr *)&addrIn, sizeof(addrIn));
  if (result != 0) {
    printf("Problem binding socket: %s\n", strerror(errno));
    exit(1);
  }

  serverContext->socket = sd;
  serverContext->openedFile = NULL;

  printf("Started server on port %d with PID %u\n", ntohs(addrIn.sin_port),
         getpid());
}

void *handleClient(struct HandleClientArgs *args) {

  printf("Hello from thread tid=%uf\n", (unsigned int)pthread_self());
  int readBytes = 0;
  struct MessageHeader *header = malloc(sizeof(struct MessageHeader));

  readBytes = messages_header_deserialize(args->rawMessage, header);

  switch ((enum MessageType)header->type) {
  case FEED_ME: {

    struct FeedMeMessage *message = malloc(sizeof(struct FeedMeMessage));
    messages_feed_me_msg_deserialize(args->rawMessage + readBytes, message);
    free(args->rawMessage);
    args->rawMessage = NULL;
    handleFeedMeMessage(args->serverContext, args->clientContext, header,
                        message);
    free(message);
    break;
  }

  default:
    printf("Can't handle that man\n");
    break;
  }

  return 0;
}

static int handleFeedMeMessage(struct ServerContext *serverContext,
                               struct ClientContext *clientContext,
                               struct MessageHeader *receivedHeader,
                               struct FeedMeMessage *receivedMessage) {

  if (serverContext->openedFile == NULL) {
    serverContext->openedFile = fopen("./audio/1.flac", "rb");
  }

  // latency simulation
  usleep(10 * 1000);

  struct MessageHeader header;
  struct DataMessage dataMessage;

  int sentBytes = 0;
  for (int i = 0; i < receivedHeader->seq; i++) {

    dataMessage.data = malloc(sizeof(char) * receivedMessage->data_size);
    dataMessage.data_size =
        fread(dataMessage.data, sizeof(char), receivedMessage->data_size,
              serverContext->openedFile);
    header.size = messages_data_msg_get_length_bytes(&dataMessage);
    header.type = DATA;
    header.seq = i;

    char buffer[FFUN_UDP_DGRAM_MAX_SIZE];

    assert((char *)&header.type != dataMessage.data + 8);
    uint16_t headerSize = messages_header_serialize(&header, buffer);
    uint messageSize = messages_data_msg_serialize(&dataMessage, buffer + headerSize);

    fwrite(dataMessage.data, sizeof(char), dataMessage.data_size, debugFile);

    free(dataMessage.data);

    int currentSend =
        sendto(serverContext->socket, buffer, (headerSize + messageSize), 0,
               (const struct sockaddr *)&clientContext->clientAddr,
               clientContext->clientAddrSize);
    sentBytes += currentSend;
  }

  printf("Sent bytes: %d\n", sentBytes);
  if (sentBytes == -1) {
    printf("error: %s\n", strerror(errno));
  }

  return 0;
}

void initializeLibrary() {
  DIR * music_dir = opendir(MUSIC_LIBRARY_PATH);
  size_t music_dir_len = strlen(MUSIC_LIBRARY_PATH);
  char path_buffer[3*265];

  memcpy(path_buffer, MUSIC_LIBRARY_PATH, music_dir_len);
  path_buffer[music_dir_len] = '/';

  if(music_dir == NULL) {
    printError("Error on opening music library dir: %s\n", strerror(errno));
  }

  struct dirent * dir;
  errno = 0;
  struct dirent * album;
  while((dir = readdir(music_dir))) {
    if(dir->d_name[0] == '.') {
      continue;
    }
    memcpy(path_buffer + music_dir_len + 1, dir->d_name, 256);
    DIR * album_dir = opendir(path_buffer);
    path_buffer[music_dir_len + 1 + strlen(dir->d_name)] = '/';
    if(album_dir == NULL) {
      printError("Error on opening album dir: %s\n", strerror(errno));
    }
    while((album = readdir(album_dir)) != NULL) {
      if(strncmp(album->d_name + strlen(album->d_name) - 4, "flac", 4)) {
        continue;
      }

      memcpy(path_buffer + music_dir_len + 1 + strlen(dir->d_name) + 1,
             album->d_name, 256);

      print_debug("%s\n", path_buffer);
    }
  }

  print_debug("Initialize library: end of loop\n");
  if(errno != 0) {
    printError("Error while iterating directory: %s\n", strerror(errno));
  }
}
