#ifndef __FFUN_CONFIG__
#define __FFUN_CONFIG__

#define FFUN_UDP_DGRAM_MAX_SIZE 508
#define FFUN_REQUESTED_DATA_SIZE 450
#define FFUN_REQUESTED_ELEMENTS_NUMBER 10
#define FFUN_REQUESTED_DATA_TIMEOUT -1

#define FFUN_FLAC_DATA_BUFF_CAPACITY 10
#define FFUN_RAW_DATA_BUFF_CAPACITY 10
#define FFUN_FLAC_DATA_BUFF_ELEMENT_SIZE 8192

// log level
#define FFUN_LOG_LEVEL_ENABLED_DEBUG
#define FFUN_LOG_LEVEL_ENABLED_VERBOSE

// player daemon
#define FFUN_PLAYER_DAEMON_DATA_MESSAGE_SERIES_SIZE 10
#define FFUN_PLAYER_DAEMON_DATA_MESSAGE_SERIES_TIMEOUT_MS 1000
#define FFUN_PLAYER_DAEMON_PORT_TCP 8091
static const char FFUN_PLAYER_DAEMON_IP[] = "0.0.0.0";

// content server
#define FFUN_CONTENT_SERVER_PORT_UDP 8080
#define FFUN_CONTENT_SERVER_PORT_TCP 8081
#define FFUN_CONTENT_SERVER_TCP_MAX_CONN 10
static const char FFUN_CONTENT_SERVER_IP[] = "0.0.0.0";

// music library
static const char MUSIC_LIBRARY_PATH[] = "/tmp/music";


#endif
