# Makefile for data structures project: Huffman code compressor/decompressor

CC=gcc

CFLAGS=-Wall -g -ansi -pedantic
#CFLAGS=-Wall -O2 -ansi -pedantic

SRCS=heap.c huffman.c util.c bitfile.c

all: hcpak

hcpak: $(SRCS:.c=.o) main.o
	$(CC) $^ -o $@

test: hcpak unittest
	@echo "Running unit tests ..."
	./unittest
	@echo
	@echo "Creating a random 10MB test file ..."
	dd if=/dev/urandom of=_test bs=1k count=10k
	@echo "Generating md5sum ..."
	md5sum _test > _test.md5
	./hcpak -v _test
	./hcpak -v -d _test.hc
	@echo "Checking ..."
	md5sum -c _test.md5
	rm -f _test _test.md5
	@echo
	@echo "All tests passed."

unittest: $(SRCS:.c=.o) unittest.o
	$(CC) $^ -o unittest

clean:
	rm -f *.o *.d hcpak unittest

%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

-include $(SRCS:.c=.d)

.PHONY: all test unittest clean
