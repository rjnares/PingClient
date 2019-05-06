CC = gcc
ARGS = -Wall

# Compiling all the dependencies
all: PingClient

PingClient: PingClient.c
	$(CC) $(ARGS) -o PingClient PingClient.c -lrt -lm

clean:
	rm -f *.o PingClient *~
