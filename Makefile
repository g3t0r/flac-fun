##
# Project Title
#
# @file
# @version 0.1

CC := gcc
LIBS := -lpthread

default: clean server

.PHONY: server
server:
	mkdir -p build
	$(CC) -g src/server.c $(LIBS) -o build/server

.PHONY: clean
clean:
	rm -rf ./build/

.PHONY: run
run:
	./build/ffun

# end
