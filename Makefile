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
	$(CC) src/main.c -o build/ffun

.PHONY: clean
clean:
	rm -rf ./build/

# end
