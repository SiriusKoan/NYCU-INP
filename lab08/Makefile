CFLAGS = -Wall -g -static
PROG = server client

all: ${PROG}

client:
	g++ ${CFLAGS} -o $@ client.cpp

server:
	g++ ${CFLAGS} -o $@ server.cpp

clean:
	rm ${PROG}
