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

default: clean server client


.PHONY: messages-test
messages-test:
	$(CC) -g src/tests/messages-test.c src/messages.c -o build/messages.test
	build/messages.test


.PHONY: server
server:
	mkdir -p build
	$(CC) -g src/server.c $(SHARED_SOURCES) $(SERVER_LIBS) -o build/server


.PHONY: client
client:
	mkdir -p build
	$(CC) -g src/client.c $(SHARED_SOURCES) $(CLIENT_LIBS) -o build/client

.PHONY: clean
clean:
	rm -rf ./build/

# end
