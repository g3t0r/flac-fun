#include "server.h"
#include "config.h"
#include "messages.h"
#include "logs.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <assert.h>
#include <bits/pthreadtypes.h>
#include <dirent.h>
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

struct AlbumEntry {
  size_t first_song_id;
  int album_size;
  char name[256];
};

struct Albums {
  size_t size;
  struct AlbumEntry * items;
};

struct SongEntry {
  size_t album_id;
  char name[256];
};

struct Songs {
  size_t size;
  struct SongEntry * items;
};

struct Library {
  struct Albums * album_list;
  struct Songs * song_list;
};

struct Server {

  // right now only single client support
  FILE * opened_file;

  struct {
    int socket;
    struct sockaddr_in addr;
  } udp_info;

  struct {
    int socket;
    struct sockaddr_in addr;
  } tcp_info;

};

struct Library * library_init();

struct Albums * library_albums(struct Library * library);

void library_album_songs(struct Library * library, size_t album_id);

const char MUSIC_LIBRARY_PATH[] = "/tmp/music";

static void server_init(struct Server * server);

static int handleFeedMeMessage(struct ServerContext *serverContext,
                               struct ClientContext *clientContext,
                               struct MessageHeader *header,
                               struct FeedMeMessage *message);

struct HandleFeedMeMsgFuncArgs {
  struct sockaddr_in client_sockaddr;
  ssize_t client_sockaddr_size;
  struct FeedMeMessage message;
};

FILE *debugFile;

int main(int argc, char **argv) {

  struct Server server;
  server_init(&server);
  print_debug("sd: %u\n", server.udp_info.socket);
  debugFile = fopen("./audio/server.data.bin", "wb");

  char message_buffer[FFUN_UDP_DGRAM_MAX_SIZE];

  while (1) {
    print_debug("Inside loop\n");

    struct pollfd server_socket_pollfd;
    server_socket_pollfd.fd = server.udp_info.socket;
    server_socket_pollfd.events = POLLIN;

    nfds_t nfds = 1;

    int result = poll(&server_socket_pollfd, nfds, -1);
    print_debug("Results: %d, R-events: %d\n", result, server_socket_pollfd.revents);

    if (result == -1) {
      print_error("Error with poll\n");
      close(server.udp_info.socket);
    }

    if (server_socket_pollfd.revents & POLLNVAL) {
      print_error("Incorrect poll request\n");
    } else if (server_socket_pollfd.revents & POLLERR) {
      print_error("Socket hung up\n");
    }

    if(result == 0) {
      continue;
    }

    if ((server_socket_pollfd.revents & POLLIN) == 0) {
      continue;
    }

    struct sockaddr_in client_sockaddr;
    struct ssize_t client_sockaddr_size;

    int read_bytes = recvfrom(server.udp_info.socket,
                              message_buffer,
                              FFUN_UDP_DGRAM_MAX_SIZE,
                              &client_sockaddr,
                              &client_sockaddr_size);

    struct MessageHeader * message_header = malloc(sizeof *message_header);
    struct HandleFeedMeMsgFuncArgs * handle_feed_me_msg_args
      = malloc(*handle_feed_me_msg_args);

    read_bytes = messages_header_deserialize(message_buffer, message_header);

    if(header.type != DATA) {
      print_error("Unsupported message type on UDP socket: %d\n", header.type);
      continue;
    }
    messages_feed_me_msg_deserialize(message_buffer + read_bytes,
                                     handle_feed_me_msg_args->message);

    /*
     * TODO: Remove this workaround after serialization of
     * segments_n field is added;
     * */

    handle_feed_me_msg_args->message.segments_n = message_header.seq;

    pthread_t tid;
    pthread_create(&tid, NULL, void *(*)(void *) handle_feed_me_msg_fn,
                   handle_feed_me_msg_args);

  }
}

void * handle_feed_me_msg_fn(struct HandleFeedMeMsgArgs * args) {}

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

struct Library * library_init() {
  DIR * music_dir = opendir(MUSIC_LIBRARY_PATH);
  size_t music_dir_len = strlen(MUSIC_LIBRARY_PATH);
  char path_buffer[3*265];

  memcpy(path_buffer, MUSIC_LIBRARY_PATH, music_dir_len);
  path_buffer[music_dir_len] = '/';

  if(music_dir == NULL) {
    print_error("Error on opening music library dir: %s\n", strerror(errno));
  }

  struct Library * library = malloc(sizeof *library);
  struct Albums * album_list = malloc(sizeof *album_list);
  struct Songs * song_list = malloc(sizeof *song_list);
  album_list->size = 10;
  album_list->items = malloc(10 * sizeof *album_list->items);
  song_list->size = 20;
  song_list->items = malloc(20 * sizeof *song_list->items);

  struct dirent * library_dirent;
  struct dirent * album_dirent;
  errno = 0;
  size_t album_count = 0;
  size_t song_count = 0;
  while((library_dirent = readdir(music_dir))) {

    if(library_dirent->d_name[0] == '.') {
      continue;
    }

    album_list->items[album_count].first_song_id = song_count;

    int album_name_len = strlen(library_dirent->d_name);

    if(album_count != 0 && album_list->size == album_count) {
      album_list->size <<= 2;
      album_list->items =
        realloc(album_list->items, album_list->size * sizeof *album_list->items);
    }

    memcpy(path_buffer + music_dir_len + 1, library_dirent->d_name, 256);
    DIR * album_dir = opendir(path_buffer);
    path_buffer[music_dir_len + 1 + album_name_len] = '/';

    while((album_dirent = readdir(album_dir))) {
      int song_name_len = strlen(album_dirent->d_name);
      if(strncmp(album_dirent->d_name + song_name_len - 4, "flac", 4)) {
        continue;
      }

      if(song_count != 0 && song_list->size == song_count) {
        song_list->size <<= 2;
        song_list->items =
          realloc(song_list->items, song_list->size * sizeof *song_list->items);
      }

      song_list->items[song_count].album_id = album_count;
      memcpy(song_list->items[song_count].name, album_dirent->d_name, 256);

      song_count++;
    }

    album_list->items[album_count].album_size
      = song_count - album_list->items[album_count].first_song_id;

    if(!album_list->items[album_count].album_size) {
      continue;
    }

    memcpy(album_list->items[album_count].name, library_dirent->d_name, 256);

    if(album_dir == NULL) {
      print_error("Error on opening album dir: %s\n", strerror(errno));
    }

    closedir(album_dir);
    album_count++;

  }

  album_list->size = album_count;
  album_list->items = realloc(album_list->items,
                              album_count* sizeof *album_list->items);

  song_list->size = song_count;
  song_list->items = realloc(song_list->items,
                              song_count* sizeof *song_list->items);

  if(errno != 0) {
    print_error("Error while iterating directory: %s\n", strerror(errno));
  }

  library->album_list = album_list;
  library->song_list = song_list;

  return library;
}

struct Albums * library_albums(struct Library * library) {
  for(int i = 0; i < library->album_list->size; i++) {
    print_debug("album: %s\n", library->album_list->items[i].name);
  }
  return NULL;
}

void library_album_songs(struct Library * library, size_t album_id) {
  struct AlbumEntry * album = library->album_list->items + album_id;
  for(int i = 0; i < album->album_size; i++) {
    struct SongEntry * song = library->song_list->items + album->first_song_id + i;
    print_debug("song: %s\n", song->name);
  }

}
