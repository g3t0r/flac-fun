##
# Project Title
#
# @file
# @version 0.1

CC := gcc
SERVER_LIBS := -lpthread -lFLAC
CLIENT_LIBS := -lFLAC -lao -ldl -lm -lpthread
CLIENT_SOURCES := src/playback.c src/circle-buffer.c
SHARED_SOURCES := src/bytes.c src/messages.c
OPTS := -g

default: clean server client

.PHONY: server
server:
	mkdir -p build
	$(CC) $(OPTS) src/server.c $(SHARED_SOURCES) $(SERVER_LIBS) -o build/server


.PHONY: client
client:
	mkdir -p build
	$(CC) $(OPTS) src/client.c $(SHARED_SOURCES) $(CLIENT_SOURCES) $(CLIENT_LIBS) -o build/client

.PHONY: clean
clean:
	rm -rf ./build/

# end
