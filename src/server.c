#include "server.h"
#include "config.h"
#include "messages.h"
#include "library.h"
#include "logs.h"
#include <arpa/inet.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
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

void print_sockaddr(struct sockaddr_in * sockaddr) {
  print_debug("sin_family: %u\n", sockaddr->sin_family);
  print_debug("sin_port: %u\n", ntohs(sockaddr->sin_port));
  print_debug("ip: %s\n", inet_ntoa(sockaddr->sin_addr));
}

static void server_udp_socket_init(struct Server * server);

static void server_udp_socket_listener_fn(struct Server * server);

static void handle_feed_me_msg_fn(struct HandleFeedMeMsgFuncArgs * args);

static int handleFeedMeMessage(struct ServerContext *serverContext,
                               struct ClientContext *clientContext,
                               struct MessageHeader *header,
                               struct FeedMeMessage *message);


FILE *debugFile;

int main(int argc, char **argv) {

  struct Server server;
  server_udp_socket_init(&server);
  library_init(&server.library);
  server.current_song_id = -1;
  print_debug("sd: %u\n", server.udp_info.socket);
  debugFile = fopen("./audio/server.data.bin", "wb");

  pthread_t udp_listener_tid;

  pthread_create(&udp_listener_tid,
                 NULL,
                 (void * (*)(void *)) server_udp_socket_listener_fn,
                 &server);

  pthread_join(udp_listener_tid, NULL);
}

void server_udp_socket_init(struct Server * server) {
  server->udp_info.socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(server->udp_info.socket == -1) {
    printf("Problem creating socket: %s\n", strerror(errno));
    exit(1);
  }


  int opt_value = 1;
  setsockopt(server->udp_info.socket, SOL_SOCKET, SO_REUSEADDR,
             &opt_value, sizeof(int));

  server->udp_info.addr.sin_family = AF_INET;
  server->udp_info.addr.sin_port = htons(FFUN_SERVER_DEFAULT_PORT);

  inet_pton(AF_INET, (const char *) &FFUN_SERVER_DEFAULT_IP,
            &server->udp_info.addr.sin_addr.s_addr);

  server->udp_info.addr.sin_addr.s_addr
    = htonl(server->udp_info.addr.sin_addr.s_addr);


  if(bind(server->udp_info.socket,
          (struct sockaddr *) &server->udp_info.addr,
          sizeof(server->udp_info.addr))) {

    printf("Problem binding socket: %s\n", strerror(errno));
    exit(1);
  }

  print_info("Server started on port %d with PID %u\n",
             FFUN_SERVER_DEFAULT_PORT,
             getpid());

}

static void handle_feed_me_msg_fn(struct HandleFeedMeMsgFuncArgs * args) {

  struct Server * server = args->server;
  struct FeedMeMessage * feed_me_message = &args->message;
  struct Library * library = &server->library;

  if(server->current_song_id != feed_me_message->song_id) {

    print_debug("Different song_id\n");
    if(server->opened_file != NULL) {
      print_debug("opened_file not NULL\n");
      fclose(server->opened_file);
    }

    char * absolute_song_path
      = library_song_build_path(library, feed_me_message->song_id);

    print_debug("Song file path: %s\n", absolute_song_path);

    server->opened_file = fopen(absolute_song_path, "rb");
    if(server->opened_file == NULL) {
      print_error("Error while opening file: %s\n", strerror(errno));
    }
    server->current_song_id = feed_me_message->song_id;
  }

  // latency simulation
  usleep(10 * 1000);

  struct MessageHeader header;
  struct DataMessage data_message;

  int sent_bytes = 0;
  for (int i = 0; i < feed_me_message->segments_n; i++) {

    data_message.data = malloc(sizeof(char) * feed_me_message->data_size);
    data_message.data_size =
      fread(data_message.data, sizeof(char), feed_me_message->data_size,
            server->opened_file);
    header.size = messages_data_msg_get_length_bytes(&data_message);
    header.type = DATA;
    header.seq = i;

    char buffer[FFUN_UDP_DGRAM_MAX_SIZE];

    assert((char *)&header.type != data_message.data + 8);
    uint16_t header_size = messages_header_serialize(&header, buffer);
    uint data_message_size = messages_data_msg_serialize(&data_message, buffer + header_size);

    fwrite(data_message.data, sizeof(char), data_message.data_size, debugFile);

    free(data_message.data);

    assert(args->client_sockaddr_size == sizeof(struct sockaddr_in));

    ssize_t sendto_result = sendto(server->udp_info.socket,
                                   buffer,
                                   (header_size + data_message_size),
                                   0,
                                   (struct sockaddr *)
                                   &args->client_sockaddr,
                                   sizeof(struct sockaddr_in));

    if(sendto_result < 0) {
      print_error("Error while sending message: %s\n", strerror(errno));
    }

  }

  printf("Sent bytes: %d\n", sent_bytes);
  if (sent_bytes == -1) {
    printf("error: %s\n", strerror(errno));
  }

}


static void server_udp_socket_listener_fn(struct Server * server) {

  char message_buffer[FFUN_UDP_DGRAM_MAX_SIZE];

  while (1) {
    print_debug("Inside loop\n");

    struct pollfd server_socket_pollfd;
    server_socket_pollfd.fd = server->udp_info.socket;
    server_socket_pollfd.events = POLLIN;

    nfds_t nfds = 1;

    int result = poll(&server_socket_pollfd, nfds, -1);
    print_debug("Results: %d, R-events: %d\n", result, server_socket_pollfd.revents);

    if (result == -1) {
      print_error("Error with poll\n");
      close(server->udp_info.socket);
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

    struct HandleFeedMeMsgFuncArgs * handle_feed_me_msg_args
      = malloc(sizeof *handle_feed_me_msg_args);

    handle_feed_me_msg_args->client_sockaddr_size = sizeof(struct sockaddr_in);


    int read_bytes = recvfrom(server->udp_info.socket,
                              message_buffer,
                              FFUN_UDP_DGRAM_MAX_SIZE,
                              0,
                              (struct sockaddr *)
                              &handle_feed_me_msg_args->client_sockaddr,
                              &handle_feed_me_msg_args->client_sockaddr_size);

    print_sockaddr(&handle_feed_me_msg_args->client_sockaddr);

    struct MessageHeader * message_header = malloc(sizeof *message_header);

    handle_feed_me_msg_args->server = server;

    read_bytes = messages_header_deserialize(message_buffer, message_header);

    if(message_header->type != FEED_ME) {
      print_error("Unsupported message type on UDP socket: %d\n",
                  message_header->type);
      continue;
    }
    messages_feed_me_msg_deserialize(message_buffer + read_bytes,
                                     &handle_feed_me_msg_args->message);

    /*
     * TODO: Remove this workaround after serialization of
     * segments_n field is added;
     * */

    handle_feed_me_msg_args->message.segments_n = message_header->seq;
    handle_feed_me_msg_args->message.song_id = 0;

    pthread_t tid;
    pthread_create(&tid, NULL, (void *(*)(void *)) handle_feed_me_msg_fn,
                   handle_feed_me_msg_args);

  }
}
