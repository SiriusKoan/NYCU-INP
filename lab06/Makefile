CFLAGS = -Wall -g
PROG = srv cli

all: ${PROG}

cli:
	gcc cli.c -lpthread -o $@

srv:
	gcc srv.c ${CFLAGS} -o $@

clean:
	rm ${PROG}
