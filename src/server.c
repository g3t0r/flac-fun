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

static void server_tcp_socket_init(struct Server * server);

static void server_udp_socket_listener_fn(struct Server * server);

static void server_tcp_socket_listener_fn(struct Server * server);

static void server_tcp_handle_connection(struct HandleTcpClientConnArgs * args);

static void handle_feed_me_msg_fn(struct HandleFeedMeMsgFuncArgs * args);

int main(int argc, char **argv) {

  struct Server server;
  server_udp_socket_init(&server);
  server_tcp_socket_init(&server);
  library_init(&server.library);
  server.current_song_id = -1;
  print_debug("sd: %u\n", server.udp_info.socket);

  pthread_t udp_listener_tid;
  pthread_t tcp_listener_tid;

  pthread_create(&udp_listener_tid,
                 NULL,
                 (void * (*)(void *)) server_udp_socket_listener_fn,
                 &server);

  pthread_create(&tcp_listener_tid,
                 NULL,
                 (void * (*)(void *)) server_tcp_socket_listener_fn,
                 &server);

  pthread_join(udp_listener_tid, NULL);
  pthread_join(tcp_listener_tid, NULL);
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

static void server_tcp_socket_init(struct Server * server) {
  server->tcp_info.socket = socket(AF_INET, SOCK_STREAM, 0);
  if(server->tcp_info.socket == -1) {
    print_error("Error while opening tcp socket: %s\n", strerror(errno));
    exit(1);
  }

  int opt_value = 1;
  setsockopt(server->tcp_info.socket, SOL_SOCKET, SO_REUSEADDR,
             &opt_value, sizeof(int));

  server->tcp_info.addr.sin_family = AF_INET;
  server->tcp_info.addr.sin_port = htons(FFUN_CONTENT_SERVER_PORT_TCP);
  inet_aton(FFUN_CONTENT_SERVER_IP, &server->tcp_info.addr.sin_addr);

  if(bind(server->tcp_info.socket,
          (struct sockaddr *) &server->tcp_info.addr,
          sizeof(struct sockaddr_in))) {

    print_error("Error while binding tcp socket: %s\n", strerror(errno));
    exit(1);
  }
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

static void server_tcp_socket_listener_fn(struct Server * server) {
  print_debug("Starting tcp socket\n");

  if(listen(server->tcp_info.socket, 5)) {
    print_error("Error on listen: %s\n", strerror(errno));
  }

  while(1) {

    struct HandleTcpClientConnArgs * client_conn_args
      = malloc(sizeof *client_conn_args);

    client_conn_args->client_sockaddr_size = sizeof(struct sockaddr_in);

    if((client_conn_args->client_socket
        = accept(server->tcp_info.socket,
                 (struct sockaddr *)
                 &client_conn_args->client_sockaddr,
                 &client_conn_args->client_sockaddr_size)) == -1) {

      print_error("Error while accepting new connection: \n");
      free(client_conn_args);
      continue;
    }

    client_conn_args->server = server;

    print_debug("New connection on TCP socket\n");
    print_sockaddr(&client_conn_args->client_sockaddr);

    pthread_t client_session_tid;
    pthread_create(&client_session_tid,
                   NULL,
                   (void * (*) (void *)) server_tcp_handle_connection,
                   client_conn_args);
  }
}

static void server_tcp_handle_connection(struct HandleTcpClientConnArgs * args) {
  print_debug("Hello from session thread\n");

  struct pollfd client_poll_fd;
  client_poll_fd.fd = args->client_socket;
  client_poll_fd.events = POLLIN;

  int client_connected = 1;

  int poll_result;
  while(client_connected) {
    poll_result = poll(&client_poll_fd, 1, -1);

    print_debug("poll_result: %d\n", poll_result);

    if(poll_result == 0) {
      print_debug("Poll result == 0");
      continue;
    }

    if(poll_result == -1) {
      print_error("Poll error on tcp socket: %s\n", strerror(errno));
      return;
    }

    if(client_poll_fd.revents & POLLERR) {
      print_error("POLLERR: %s\n", strerror(errno));
      break;
    }

    if(client_poll_fd.revents & POLLHUP) {
      print_debug("POLLHUP:\n");
      break;
    }

    int read_bytes = 0;

    char buffer[4096];
    struct MessageHeader header;
    read_bytes = recv(client_poll_fd.fd, buffer + read_bytes,
                      MSG_HEADER_SIZE - read_bytes, 0);

    if(read_bytes == 0) {
      break;
    }

    messages_header_deserialize(buffer, &header);

    print_debug("Received header: type: %d, size: %d, seq: %d\n",
                header.type, header.size, header.seq);

    if(header.type == MESSAGE_TYPE_ALBUM_LIST_REQ) {
      struct LibraryAlbums * lib_album_list = library_albums(&args->server->library);
      struct AlbumListMessage msg_album_list;
      msg_album_list.size = lib_album_list->size;
      msg_album_list.album_list
        = malloc(lib_album_list->size * sizeof(*msg_album_list.album_list));

      for(int i = 0; i < lib_album_list->size; i++) {
        const struct LibraryAlbumEntry * lib_album_entry = lib_album_list->items + i;
        struct AlbumListEntry * msg_album_entry = msg_album_list.album_list + i;

        msg_album_entry->album_id = i;
        msg_album_entry->album_name_size = strlen(lib_album_entry->name) + 1;
        msg_album_entry->album_name = (char *) lib_album_entry->name;
      }

      int response_size = sizeof(header)
        + messages_album_list_resp_msg_get_length_bytes(&msg_album_list);

      char * response_buffer = malloc(response_size);
      header.seq = 0;
      header.type = MESSAGE_TYPE_ALBUM_LIST_RESP;
      header.size = response_size;

      int written_bytes = messages_header_serialize(&header, response_buffer);
      written_bytes += messages_album_list_resp_msg_serialize(
        &msg_album_list, response_buffer + written_bytes);

      print_debug("Free msg_album_list->items\n");
      free(msg_album_list.album_list);

      int sent_bytes = 0;
      while(sent_bytes != written_bytes) {
        sent_bytes += send(args->client_socket,
                           response_buffer + sent_bytes,
                           written_bytes - sent_bytes, 0);
      }
    }

    if(header.type == MESSAGE_TYPE_ALBUM_SONGS_REQ) {
      struct AlbumSongsReqMessage album_songs_req_msg;

      read_bytes = 0;
      while(read_bytes != MSG_ALBUM_SONGS_REQ_SIZE) {
        int local_read_bytes = recv(args->client_socket,
                        buffer + read_bytes,
                        MSG_ALBUM_SONGS_REQ_SIZE - read_bytes,
                        0);

        read_bytes += local_read_bytes;
        print_debug("Bytes: %d/%d\n", read_bytes, MSG_ALBUM_SONGS_REQ_SIZE);

      }

      messages_album_songs_req_msg_deserialize(
        buffer, &album_songs_req_msg);

      print_debug("Song_id: %d\n", album_songs_req_msg.album_id);

      struct LibrarySongs * lib_song_list = library_album_songs(
        &args->server->library,
        album_songs_req_msg.album_id);

      struct AlbumSongsRespMessage album_song_resp_msg;
      album_song_resp_msg.size = lib_song_list->size;
      album_song_resp_msg.items
        = malloc(album_song_resp_msg.size * sizeof *album_song_resp_msg.items);

      size_t fist_song_id = lib_song_list->first_song_id;
      for(int i = 0; i < album_song_resp_msg.size; i++) {
        struct LibrarySongEntry * lib_song = lib_song_list->items + i;
        struct AlbumSongItem * msg_song = album_song_resp_msg.items + i;

        msg_song->song_id = lib_song_list->first_song_id + i;
        msg_song->song_name = (char *) lib_song->name;
        msg_song->song_name_size = strlen(lib_song->name) + 1;
      }

      int response_size = MSG_HEADER_SIZE
        + messages_album_songs_resp_get_length_bytes(&album_song_resp_msg);

      char * response_buffer = malloc(response_size);
      header.seq = 0;
      header.size = response_size;
      header.type = MESSAGE_TYPE_ALBUM_SONGS_RESP;

      int written_bytes = messages_header_serialize(
        &header,
        response_buffer);

      assert(written_bytes == MSG_HEADER_SIZE);

      written_bytes += messages_album_songs_resp_serialize(
        &album_song_resp_msg,
        response_buffer + written_bytes);

      int sent_bytes = 0;

      while(sent_bytes != response_size) {
        print_debug("%d bytes left\n", response_size - sent_bytes);
        sent_bytes += send(args->client_socket,
                           response_buffer + sent_bytes,
                           response_size - sent_bytes, 0);
      }

    }


    print_debug("Read_bytes: %d\n", read_bytes);


  }
  print_debug("Client disconnected\n");
}
