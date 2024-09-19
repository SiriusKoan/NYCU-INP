CFLAGS = -Wall -g
PROG = tcp-cli tcp-srv-fork tcp-srv-iomul

all: ${PROG}

tcp-cli:
	gcc ${CFLAGS} tcp-cli.c -o $@

tcp-srv-fork:
	gcc ${CFLAGS} tcp-srv-fork.c -o $@

tcp-srv-iomul:
	gcc ${CFLAGS} tcp-srv-iomul.c -o $@

clean:
	rm ${PROG}
