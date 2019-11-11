PROG = makebaby
CC = gcc
# Need to use -isystem here to ignore warnings in elfutils headers
CFLAGS = -g -pedantic -Wall -Werror -Wextra \
         -Wstrict-prototypes -Wold-style-definition \
		 -std=c99 
HFILES = $(wildcard *.h)
LIBS =
CFILES = $(wildcard *.c)
OFILES = $(patsubst %.c,%.o,$(CFILES))

.PHONY: all clean

all: $(PROG)

$(PROG): $(OFILES)
	$(CC) -o $@ $^ $(LIBS)

%.o: %.c $(HFILES)
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(PROG)
