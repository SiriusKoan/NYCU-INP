CFLAGS = -Wall -g -static
PROG = server client

all: ${PROG}

client:
	gcc ${CFLAGS} client.c -o $@

server:
	gcc ${CFLAGS} server.c -o $@

clean:
	rm ${PROG}
