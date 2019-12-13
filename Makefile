CC = gcc

CFLAGS += -Wall -fno-stack-protector -Wextra -g

.PHONY: all clean

# Build
all: release

release: server clientRelease

server:
	$(CC) $(CFLAGS) server.c -o pserver $(LIBS)

clientRelease:
	$(CC) $(CFLAGS) client.c -o client $(LIBS)

clean:
	-rm -f pserver client
