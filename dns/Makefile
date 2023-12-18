CC	= gcc
CFLAGS	= -O2 -Wall -Werror -pedantic -std=gnu99
LIBS	= -lm -lpthread

PROGRAM	= dnscmd

$(PROGRAM): main.c
	$(CC) $(CFLAGS) $(LIBS) -o $@ main.c
