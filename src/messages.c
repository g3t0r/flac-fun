#include "messages.h"
#include "bytes.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * Function converts MessageHeader into big endian bytes, and stored them
 *          into buffer
 *
 * @param message Pointer to MessageHeader.
 *
 * @param buffer Buffer in which bytes should be stored. Memory under this
 *               pointer has to be already allocated
 *
 * @return Number of bytes written to the buffer
 */
uint32_t messages_header_serialize(const struct MessageHeader *const header,
                                char *buffer) {
  int written_bytes =
      bytes_buffer_write_int(buffer, &header->type, sizeof(header->type));

  written_bytes += bytes_buffer_write_int(buffer + written_bytes, &header->size,
                                       sizeof(header->size));

  written_bytes += bytes_buffer_write_int(buffer + written_bytes, &header->seq,
                                       sizeof(header->seq));

  return written_bytes;
}

/**
 * Function convert bytes from buffer to MessageHeader.
 *
 * @param buffer Buffer of bytes stored in big endian
 *
 * @param message Pointer to message that bytes should be read into.
 *                Should be already allocated.
 *
 * @return Number of bytes read from the buffer
 */
uint32_t messages_header_deserialize(const char *const buffer,
                                  struct MessageHeader *header) {
  int read_bytes =
      bytes_buffer_read_int(&header->type, buffer, sizeof(header->type));

  read_bytes += bytes_buffer_read_int(&header->size, buffer + read_bytes,
                                     sizeof(header->size));

  read_bytes += bytes_buffer_read_int(&header->seq, buffer + read_bytes,
                                     sizeof(header->seq));

  return read_bytes;
}

/**
 * Function converts DataMessage into big endian bytes, and stored them into
 * buffer
 *
 * @param message Pointer to DataMessage
 *
 * @param buffer Buffer in which bytes should be stored. Memory under this
 *               pointer has to be already allocated
 *
 * @return Number of bytes written to the buffer
 */
uint32_t messages_data_msg_serialize(const struct DataMessage *const message,
                              char *buffer) {
  int written_bytes = bytes_buffer_write_int(buffer, &message->data_size,
                                          sizeof(message->data_size));

  memcpy(buffer + written_bytes, message->data, message->data_size);
  written_bytes += message->data_size;

  return written_bytes;
}

/**
 * Function convert bytes from buffer to DataMessage
 *
 * @param buffer Buffer of bytes stored in big endian
 * @param message Pointer to message that bytes should be read into.
 *                Should be already allocated.
 *
 * @return Number of bytes read from the buffer
 */
uint32_t messages_data_msg_deserialize(const char *const buffer,
                                struct DataMessage *message) {

  int read_bytes = bytes_buffer_read_int(&message->data_size, buffer,
                                        sizeof(message->data_size));

  message->data = malloc(sizeof(char) * message->data_size);
  memcpy(message->data, buffer + read_bytes, message->data_size);
  read_bytes += message->data_size;

  return read_bytes;
}

/**
 * Function calculates number of bytes needed to serialize DataMessage,
 *          including memory pointed by DataMessage.data
 */
uint32_t messages_data_msg_get_length_bytes(const struct DataMessage *const message) {
  return sizeof(message->data_size) + message->data_size * sizeof(char);
}

uint16_t messages_feed_me_msg_serialize(const struct FeedMeMessage *message,
                                char *buffer) {
  return bytes_buffer_write_int(buffer, &message->data_size,
                              sizeof(message->data_size));
}

uint16_t messages_feed_me_msg_deserialize(const char *const buffer,
                                  struct FeedMeMessage *message) {
  return bytes_buffer_read_int(&message->data_size, buffer,
                               sizeof(message->data_size));
}

uint32_t messages_play_song_msg_serialize(
    const struct PlaySongMessage * message,
    char * buffer) {
  return bytes_buffer_write_int(buffer, &message->song_id, sizeof(message->song_id));
}

uint32_t messages_play_song_msg_deserialize(const char * const buffer,
    struct PlaySongMessage * message) {
  return bytes_buffer_read_int(&message->song_id, buffer, sizeof(message->song_id));
}


uint32_t messages_album_list_msg_resp_serialize
(const struct AlbumListMessage * const message,
 char * buffer) {

  uint32_t written_bytes = bytes_buffer_write_int(buffer, &message->size, sizeof(message->size));
  for(int i = 0; i < message->size; i++) {

    written_bytes += bytes_buffer_write_int(buffer + written_bytes,
                                            &message->album_list[i].album_id,
                                            sizeof(message->album_list[i].album_id));

    written_bytes += bytes_buffer_write_int(buffer + written_bytes,
                                            &message->album_list[i].album_name_size,
                                            sizeof(message->album_list[i]
                                                   .album_name_size));

    written_bytes += bytes_buffer_write_int(buffer + written_bytes,
                                            message->album_list[i].album_name,
                                            message->album_list[i].album_name_size);
  }
  return written_bytes;
}


uint32_t messages_album_list_msg_resp_deserialize(const char * const buffer,
                                                struct AlbumListMessage * message) {
  uint32_t read_bytes = bytes_buffer_read_int(&message->size, buffer, sizeof(message->size));

  message->album_list = malloc(message->size * sizeof *message->album_list);
  struct AlbumListEntry * entry = message->album_list;

  while(entry - message->album_list < message->size) {

    read_bytes += bytes_buffer_read_int(&entry->album_id, buffer + read_bytes,
                                        sizeof(entry->album_id));

    read_bytes += bytes_buffer_read_int(&entry->album_name_size, buffer + read_bytes,
                                        sizeof(entry->album_id));

    entry->album_name = malloc(entry->album_name_size);

    memcpy(entry->album_name, buffer + read_bytes, entry->album_name_size);

    read_bytes += entry->album_name_size;

    entry++;
  }

  return read_bytes;
}

uint32_t messages_album_list_msg_get_length_bytes(const struct AlbumListMessage * const message) {
  uint32_t bytes_total = sizeof(message->size);
  struct AlbumListEntry * entry = message->album_list;
  while(entry - message->album_list < message->size) {
    bytes_total += sizeof(entry->album_id)
      + sizeof(entry->album_name_size)
      + entry->album_name_size;
  }
  return bytes_total;
}
