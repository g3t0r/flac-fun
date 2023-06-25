##
# Project Title
#
# @file
# @version 0.1

CC := gcc

default: clean compile

.PHONY: compile
compile:
	mkdir -p build
	$(CC) -g src/main.c -lFLAC -o build/ffun

.PHONY: clean
clean:
	rm -rf ./build/

.PHONY: run
run:
	./build/ffun

# end
