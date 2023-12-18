CC	= gcc
CFLAGS	= -O2 -Wall -Werror -pedantic -std=gnu99
LIBS	= -lm -lpthread -no-pie

PROGRAM	= httpcmd

$(PROGRAM): main.c http.a
	$(CC) $(CFLAGS) $(LIBS) -o $@ main.c http.a

http.a:
	$(CC) -c $(CFLAGS) $(LIBS) -o $@ http.c
