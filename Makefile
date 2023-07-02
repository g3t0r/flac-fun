##
# Project Title
#
# @file
# @version 0.1

CC := gcc
SERVER_LIBS := -lpthread
CLIENT_LIBS :=

default: clean server client

.PHONY: server
server:
	mkdir -p build
	$(CC) -g src/server.c $(SERVER_LIBS) -o build/server


.PHONY: client
client:
	mkdir -p build
	$(CC) -g src/client.c $(CLIENT_LIBS) -o build/client

.PHONY: clean
clean:
	rm -rf ./build/

# end
