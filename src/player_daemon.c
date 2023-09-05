#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>

#include "bytes.h"
#include "circle-buffer.h"
#include "config.h"
#include "logs.h"
#include "messages.h"
#include "playback.h"
#include "time.h"
#include "unistd.h"

enum PlayerDaemonAudioStatus {
  PLAYER_DAEMON_AUDIO_STATUS_STOPPED = 0,
  PLAYER_DAEMON_AUDIO_STATUS_PLAYING
};

enum PollIndex {
  POLL_INDEX_UDP = 0,
  POLL_INDEX_TCP
};

struct PlayerDaemon {
  int socket;
  struct sockaddr_in sock_addr;
  enum PlayerDaemonAudioStatus player_status;
  struct Playback * playback;
  struct DataMessage message_buffer[FFUN_PLAYER_DAEMON_DATA_MESSAGE_SERIES_SIZE];
  uint message_series_element_counter;
  sem_t message_series_element_counter_mutex;
  sem_t message_series_data_merge_mutex;
  struct {
    int socket;
    struct sockaddr_in sock_addr;
  } content_server;
};

void * player_daemon_request_data_loop_thread_fn(struct PlayerDaemon * player_daemon);
void player_daemon_handle_data_message(struct PlayerDaemon * player_daemon, struct MessageHeader * message_header, struct DataMessage * data_message);

int main(int argc, char** argv) {

  struct PlayerDaemon player_daemon;

  player_daemon.player_status = PLAYER_DAEMON_AUDIO_STATUS_PLAYING;
  player_daemon.message_series_element_counter = 0;
  sem_init(&player_daemon.message_series_element_counter_mutex, 0, 1);
  sem_init(&player_daemon.message_series_data_merge_mutex, 0, 0);

  player_daemon.content_server.socket = socket(AF_INET, SOCK_DGRAM, 0);
  player_daemon.content_server.sock_addr.sin_family = AF_INET;
  player_daemon.content_server.sock_addr.sin_port = htons(FFUN_CONTENT_SERVER_PORT_UDP);
  inet_aton(FFUN_CONTENT_SERVER_IP,
      &player_daemon.content_server.sock_addr.sin_addr);

  bind(player_daemon.content_server.socket,
      (struct sockaddr *) &player_daemon.content_server.sock_addr,
      sizeof(struct sockaddr_in));


  struct pollfd poll_fd[2];
  poll_fd[POLL_INDEX_UDP].fd = player_daemon.content_server.socket;
  poll_fd[POLL_INDEX_UDP].events = POLLIN;

  char udp_data_buffer[FFUN_UDP_DGRAM_MAX_SIZE];

  pthread_t data_request_loop_tid;
  pthread_create(&data_request_loop_tid, NULL,
      (void * (*)(void *))player_daemon_request_data_loop_thread_fn,
      &player_daemon);

  player_daemon.playback = malloc(sizeof(struct Playback));
  playback_init(player_daemon.playback);
  pthread_t playback_t;
  pthread_create(&playback_t, NULL,
     (void * (*)(void *)) playback_start, player_daemon.playback);

  while(1) {
    print_debug("Listener loop iteration\n");
    int poll_result = poll(poll_fd, 2, -1);

    if(poll_result == -1) {
     printError("Error durring poll(), message: %s\n", strerror(errno));
    }

    ssize_t read_bytes = read(poll_fd[POLL_INDEX_UDP].fd, udp_data_buffer, FFUN_UDP_DGRAM_MAX_SIZE);

    if(read_bytes == -1) {
     printError("Error durring read(), message: %s\n", strerror(errno));
    }

    struct MessageHeader * header = malloc(sizeof *header);

    struct DataMessage * data_message = malloc(sizeof *header);
    size_t header_size = messages_header_deserialize(udp_data_buffer, header);
    if(header->type != DATA) {
      printError("Not supported yet\n");
    }
    print_debug("Received data message\n");
    messages_data_msg_deserialize(udp_data_buffer + header_size, data_message);
    player_daemon_handle_data_message(&player_daemon, header, data_message);
  }

  return 0;
}

void player_daemon_handle_data_message(
    struct PlayerDaemon * player_daemon,
    struct MessageHeader * message_header,
    struct DataMessage * data_message) {

  player_daemon->message_buffer[message_header->seq] = *data_message;
  player_daemon->message_series_element_counter++;

  if(player_daemon->message_series_element_counter == FFUN_PLAYER_DAEMON_DATA_MESSAGE_SERIES_SIZE) {
    sem_post(&player_daemon->message_series_data_merge_mutex);
  }
}

void * player_daemon_request_data_loop_thread_fn(struct PlayerDaemon * player_daemon) {

  struct FeedMeMessage feed_me_message = { FFUN_REQUESTED_DATA_SIZE };
  struct MessageHeader message_header = {
    FFUN_PLAYER_DAEMON_DATA_MESSAGE_SERIES_SIZE,
    sizeof(message_header) + sizeof(feed_me_message),
    FEED_ME
  };

  char udp_data_buffer[FFUN_UDP_DGRAM_MAX_SIZE];
  size_t udp_data_size =  messages_header_serialize(&message_header, udp_data_buffer);
  udp_data_size += messages_feed_me_msg_serialize(&feed_me_message,
      udp_data_buffer + udp_data_size);

  while(player_daemon->player_status == PLAYER_DAEMON_AUDIO_STATUS_PLAYING) {
    
    sendto(player_daemon->content_server.socket,
        udp_data_buffer,
        udp_data_size,
        0,
        (struct sockaddr *) &player_daemon->content_server.sock_addr,
        sizeof(player_daemon->content_server.sock_addr));

    struct timespec time_spec;
    clock_gettime(CLOCK_REALTIME, &time_spec);
    time_spec.tv_sec += 3600;
    time_spec.tv_nsec += 1000000 /* nsec in milisec*/
      * FFUN_PLAYER_DAEMON_DATA_MESSAGE_SERIES_SIZE;

    /*
    if(ETIMEDOUT == sem_timedwait(
          &player_daemon->message_series_data_merge_mutex,
          &time_spec)) {
      printVerbose("message_series_data_merge_mutex timed out\n");
    }
    */

    sem_wait(&player_daemon->message_series_data_merge_mutex);
    char message_series_merge_buffer[FFUN_PLAYER_DAEMON_DATA_MESSAGE_SERIES_SIZE
      * FFUN_UDP_DGRAM_MAX_SIZE];

    int merged_data_size = 0;
    for(int i = 0; i < player_daemon->message_series_element_counter; i++) {
      memcpy(message_series_merge_buffer + merged_data_size,
          (player_daemon->message_buffer + i)->data,
          (player_daemon->message_buffer + i)->dataSize);

      merged_data_size += (player_daemon->message_buffer + i)->dataSize;
    }

    player_daemon->message_series_element_counter = 0;

    print_debug("before feeding data\n");
    playback_feed_data(
        player_daemon->playback,
        message_series_merge_buffer,
        merged_data_size);
    print_debug("after feeding data\n");
  }

  return NULL;
}
