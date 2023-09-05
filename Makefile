##
# Flac Fun
#
# @file
# @version 0.1

CC := gcc
SERVER_LIBS := -lpthread -lFLAC
CLIENT_LIBS := -lFLAC -lao -ldl -lm -lpthread
CLIENT_SOURCES := src/playback.c src/circle-buffer.c
SHARED_SOURCES := src/bytes.c src/messages.c
OPTS := -g -pedantic

default: clean server player_daemon cli

.PHONY: server
server:
	mkdir -p build
	$(CC) $(OPTS) src/server.c $(SHARED_SOURCES) $(SERVER_LIBS) -o build/server


.PHONY: player_daemon
player_daemon:
	mkdir -p build
	$(CC) $(OPTS) src/player_daemon.c $(SHARED_SOURCES) $(CLIENT_SOURCES) $(CLIENT_LIBS) -o build/player_daemon

.PHONY: cli
cli:
	mkdir -p build
	$(CC) $(OPTS) src/cli.c src/player_client.c $(SHARED_SOURCES) -o build/ffcli

.PHONY: clean
clean:
	rm -rf ./build/

# end
