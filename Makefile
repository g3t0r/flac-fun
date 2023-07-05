##
# Project Title
#
# @file
# @version 0.1

CC := gcc
SERVER_LIBS := -lpthread
CLIENT_LIBS :=
SHARED_SOURCES := src/messages.c
LINKED_OBJECTS := build/messages.o

default: clean messages server client

.PHONY: messages
messages:
	mkdir -p build
	$(CC) -g -c src/messages.c -o build/messages.o

.PHONY: messages-test
messages-test:
	mkdir -p build
	$(CC) -g -DTESTMESSAGES src/messages.c -o build/messages.test
	build/messages.test


.PHONY: server
server:
	mkdir -p build
	$(CC) -g src/server.c -r $(LINKED_OBJECTS) $(SERVER_LIBS) -o build/server


.PHONY: client
client:
	mkdir -p build
	$(CC) -g src/client.c -r $(LINKED_OBJECTS) $(CLIENT_LIBS) -o build/client

.PHONY: clean
clean:
	rm -rf ./build/

# end
