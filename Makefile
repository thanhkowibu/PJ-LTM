CC = gcc

CFLAGS = -g
LDFLAGS = -ljson-c

TARGET1 = server

all: $(TARGET1)

$(TARGET1): server.o
	$(CC) $(CFLAGS) -o $(TARGET1) server.o $(LDFLAGS)

server.o: server.c
	$(CC) $(CFLAGS) -c server.c

clean:
	rm -f *.o $(TARGET1)
