#include <assert.h>
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

#define sem_wait(sem) printf("sem_wait: %s:%d\n", __FILE__, __LINE__); sem_wait(sem);

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
  uint32_t song_id;
  sem_t data_msg_socket_mutex;
  pthread_t data_request_loop_tid;
};

void * player_daemon_request_data_loop_thread_fn(struct PlayerDaemon * player_daemon);
void player_daemon_handle_data_message(struct PlayerDaemon * player_daemon, int udp_socket);
void player_daemon_handle_control_message( struct PlayerDaemon * player_daemon, int tcp_socket);

int main(int argc, char** argv) {

  struct PlayerDaemon player_daemon;

  player_daemon.player_status = PLAYER_DAEMON_AUDIO_STATUS_PLAYING;
  player_daemon.message_series_element_counter = 0;
  sem_init(&player_daemon.message_series_element_counter_mutex, 0, 1);
  sem_init(&player_daemon.message_series_data_merge_mutex, 0, 0);
  sem_init(&player_daemon.data_msg_socket_mutex, 0, 1);


  // tcp socket
  player_daemon.socket = socket(AF_INET, SOCK_STREAM, 0);
  player_daemon.sock_addr.sin_family = AF_INET;
  player_daemon.sock_addr.sin_port = htons(FFUN_PLAYER_DAEMON_PORT_TCP);
  inet_aton("0.0.0.0",
      &player_daemon.sock_addr.sin_addr);

  if(bind(player_daemon.socket, (struct sockaddr *) &player_daemon.sock_addr,
      sizeof(struct sockaddr_in)) == -1) {
    print_error("Problem with binding tcp socket: %s\n", strerror(errno));
    exit(1);
  }

  if(listen(player_daemon.socket, 20)) {
    print_error("Problem with listening on tcp socket: %s\n", strerror(errno));
    exit(1);
  }

  // udp socket
  player_daemon.content_server.socket = socket(AF_INET, SOCK_DGRAM, 0);
  player_daemon.content_server.sock_addr.sin_family = AF_INET;
  player_daemon.content_server.sock_addr.sin_port = htons(FFUN_CONTENT_SERVER_PORT_UDP);
  inet_aton(FFUN_CONTENT_SERVER_IP,
      &player_daemon.content_server.sock_addr.sin_addr);

  struct pollfd poll_fd[2];
  poll_fd[POLL_INDEX_UDP].fd = player_daemon.content_server.socket;
  poll_fd[POLL_INDEX_UDP].events = POLLIN;
  
  poll_fd[POLL_INDEX_TCP].fd = player_daemon.socket;
  poll_fd[POLL_INDEX_TCP].events = POLLIN;


  player_daemon.playback = malloc(sizeof(struct Playback));
  playback_init(player_daemon.playback);

  while(1) {
    int poll_result = poll(poll_fd, 2, -1);

    if(poll_result == -1) {
     print_error("Error durring poll(), message: %s\n", strerror(errno));
    }

    if(poll_fd[POLL_INDEX_TCP].revents & POLLIN) {
      player_daemon_handle_control_message(&player_daemon, poll_fd[POLL_INDEX_TCP].fd);
    }

    if(poll_fd[POLL_INDEX_UDP].revents & POLLIN) {
      player_daemon_handle_data_message(&player_daemon, poll_fd[POLL_INDEX_UDP].fd);
    }
  }


  return 0;
}


void player_daemon_handle_control_message(
  struct PlayerDaemon * player_daemon,
  int tcp_socket) {

  print_debug("Received message on TCP, more info soon\n");
  int read_bytes = 0;

  char * tcp_data_buffer = malloc(MSG_HEADER_SIZE);

  struct sockaddr_in client_sockaddr;
  socklen_t client_sockaddr_size;
  int accepted_socket =  accept(tcp_socket, (struct sockaddr *) &client_sockaddr, &client_sockaddr_size);
  while(read_bytes != MSG_HEADER_SIZE) {
    read_bytes += recv(accepted_socket, tcp_data_buffer + read_bytes, MSG_HEADER_SIZE - read_bytes, 0);
    if(read_bytes == 0) {
      print_debug("Client disconnected\n");
      return;
    }
  }

  struct MessageHeader header;

  messages_header_deserialize(tcp_data_buffer, &header);

  switch((enum MessageType) header.type) {
    case MESSAGE_TYPE_PLAY_SONG: {
      tcp_data_buffer = realloc(tcp_data_buffer, MSG_PLAY_SONG_SIZE);
      recv(accepted_socket, tcp_data_buffer, MSG_PLAY_SONG_SIZE, 0);
      struct PlaySongMessage play_song_msg;
      messages_play_song_msg_deserialize(tcp_data_buffer,
                                         &play_song_msg);

      print_debug("Play song: %d\n", play_song_msg.song_id);
      player_daemon->song_id = play_song_msg.song_id;
      player_daemon->player_status = PLAYER_DAEMON_AUDIO_STATUS_PLAYING;

      pthread_create(&player_daemon->data_request_loop_tid, NULL,
                     (void * (*)(void *))player_daemon_request_data_loop_thread_fn,
                     player_daemon);

      playback_start(player_daemon->playback);

      print_debug("After playback start\n");
      break;
    }
    case MESSAGE_TYPE_PAUSE: {
      print_debug("Pause command\n");
      playback_pause(player_daemon->playback);
      break;
    }
    case MESSAGE_TYPE_RESUME: {
      print_debug("Resume command\n");
      playback_resume(player_daemon->playback);
      break;
    }
    case MESSAGE_TYPE_STOP: {
      print_debug("Stop command\n");

      player_daemon->player_status = PLAYER_DAEMON_AUDIO_STATUS_STOPPED;
      sem_post(&player_daemon->message_series_element_counter_mutex);
      sem_post(&player_daemon->message_series_element_counter_mutex);
      sem_post(&player_daemon->message_series_element_counter_mutex);
      sem_post(&player_daemon->message_series_element_counter_mutex);
      sem_post(&player_daemon->message_series_element_counter_mutex);
      sem_post(&player_daemon->message_series_element_counter_mutex);
      sem_post(&player_daemon->message_series_element_counter_mutex);
      sem_post(&player_daemon->message_series_element_counter_mutex);
      sem_post(&player_daemon->message_series_element_counter_mutex);
      sem_post(&player_daemon->message_series_element_counter_mutex);
      sem_post(&player_daemon->message_series_element_counter_mutex);

      sem_post(&player_daemon->message_series_data_merge_mutex);
      sem_post(&player_daemon->message_series_data_merge_mutex);
      sem_post(&player_daemon->message_series_data_merge_mutex);
      sem_post(&player_daemon->message_series_data_merge_mutex);
      sem_post(&player_daemon->message_series_data_merge_mutex);
      sem_post(&player_daemon->message_series_data_merge_mutex);
      sem_post(&player_daemon->message_series_data_merge_mutex);
      sem_post(&player_daemon->message_series_data_merge_mutex);
      sem_post(&player_daemon->message_series_data_merge_mutex);
      sem_post(&player_daemon->message_series_data_merge_mutex);
      sem_post(&player_daemon->message_series_data_merge_mutex);

      print_debug("Before joining data thread\n");
      pthread_join(player_daemon->data_request_loop_tid, NULL);
      playback_stop(player_daemon->playback);
      playback_reset(player_daemon->playback);
      print_debug("Joined data thread\n");

      player_daemon->message_series_element_counter = 0;
      sem_destroy(&player_daemon->message_series_element_counter_mutex);
      sem_destroy(&player_daemon->message_series_data_merge_mutex);
      sem_init(&player_daemon->message_series_data_merge_mutex, 0, 0);

      break;
    }
    default:
      print_error("Not supported message type: %d\n", header.type);
      break;
  }


}

void player_daemon_handle_data_message(
  struct PlayerDaemon * player_daemon,
  int udp_socket) {

  char udp_data_buffer[FFUN_UDP_DGRAM_MAX_SIZE];
  ssize_t read_bytes = read(udp_socket, udp_data_buffer, FFUN_UDP_DGRAM_MAX_SIZE);

  if(read_bytes == -1) {
    print_error("Error durring read(), message: %s\n", strerror(errno));
  }

  struct MessageHeader header;
  size_t header_size = messages_header_deserialize(udp_data_buffer, &header);
  if(header.type != DATA) {
    print_error("Not supported yet\n");
  }

  print_debug("Counter: %d\n", player_daemon->message_series_element_counter);
  print_debug("Header seq: %d\n", header.seq);

  struct DataMessage data_message_tmp;

  messages_data_msg_deserialize(udp_data_buffer + header_size,
                                &data_message_tmp);

  if(data_message_tmp.song_id != player_daemon->song_id) {
    print_debug("Received old data message, ignoring\n");
    return;
  }

  print_debug("DataMessage.song_id: %d\n", data_message_tmp.song_id);

  player_daemon->message_buffer[player_daemon->message_series_element_counter] = data_message_tmp;

  print_debug("Before problematic wait\n");
  if(player_daemon->player_status != PLAYER_DAEMON_AUDIO_STATUS_PLAYING) {
    print_debug("MAYBE WILL RETURN???? \n");
    return;
  }
  sem_wait(&player_daemon->message_series_element_counter_mutex);
  if(player_daemon->player_status != PLAYER_DAEMON_AUDIO_STATUS_PLAYING) {
    return;
  }
  print_debug("Successful wait\n");
  player_daemon->message_series_element_counter++;

  if(player_daemon->message_series_element_counter == FFUN_PLAYER_DAEMON_DATA_MESSAGE_SERIES_SIZE) {
    sem_post(&player_daemon->message_series_data_merge_mutex);
  }
  sem_post(&player_daemon->message_series_element_counter_mutex);
}

void * player_daemon_request_data_loop_thread_fn(struct PlayerDaemon * player_daemon) {

  struct FeedMeMessage feed_me_message;
  feed_me_message.data_size = FFUN_REQUESTED_DATA_SIZE;
  feed_me_message.song_id = player_daemon->song_id;


  struct MessageHeader message_header;
  message_header.seq = FFUN_PLAYER_DAEMON_DATA_MESSAGE_SERIES_SIZE;
  message_header.size = MSG_HEADER_SIZE + MSG_FEED_ME_SIZE;
  message_header.type = FEED_ME;

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
    print_debug("Time spec: %lus %luns\n", time_spec.tv_sec, time_spec.tv_nsec);
    time_spec.tv_nsec += 20000000
      * FFUN_PLAYER_DAEMON_DATA_MESSAGE_SERIES_SIZE;
    print_debug("Time spec: %lus %luns\n", time_spec.tv_sec, time_spec.tv_nsec);

    time_spec.tv_sec += time_spec.tv_nsec / 1000000000;
    time_spec.tv_nsec %= 1000000000;
    print_debug("Time spec: %lus %luns\n", time_spec.tv_sec, time_spec.tv_nsec);



    if(-1 == sem_timedwait(
         &player_daemon->message_series_data_merge_mutex,
         &time_spec)) {
      print_error("timedwait error: (%d), %s\n", errno, strerror(errno));
    }



    //sem_wait(&player_daemon->message_series_data_merge_mutex);
    if(player_daemon->player_status != PLAYER_DAEMON_AUDIO_STATUS_PLAYING) {
      return NULL;
    }
    char message_series_merge_buffer[FFUN_PLAYER_DAEMON_DATA_MESSAGE_SERIES_SIZE
                                     * FFUN_UDP_DGRAM_MAX_SIZE];

    int merged_data_size = 0;
    assert(player_daemon->message_series_element_counter != 0);
    for(int i = 0; i < player_daemon->message_series_element_counter; i++) {
      if((player_daemon->message_buffer + i)->data_size == 0) {
        print_debug("Detected 0 size at index: %d\n", i);
      }
      memcpy(message_series_merge_buffer + merged_data_size,
             (player_daemon->message_buffer + i)->data,
             (player_daemon->message_buffer + i)->data_size);

      merged_data_size += (player_daemon->message_buffer + i)->data_size;
    }

    print_debug("B E F O R E: Waiting in thread\n");
    sem_wait(&player_daemon->message_series_element_counter_mutex);
    print_debug("A F T E R: Waiting in thread\n");
    if(player_daemon->player_status != PLAYER_DAEMON_AUDIO_STATUS_PLAYING) {
      print_debug("R E T U R N\n");
      return NULL;
    }
    player_daemon->message_series_element_counter = 0;
    sem_post(&player_daemon->message_series_element_counter_mutex);

    print_debug("Merged data size: %d\n", merged_data_size);
    assert(merged_data_size != 0);
    print_debug("Waiting to feed\n");
    playback_feed_data(
      player_daemon->playback,
      message_series_merge_buffer,
      merged_data_size);
    print_debug("Feeding finished\n");
  }

  return NULL;
}
