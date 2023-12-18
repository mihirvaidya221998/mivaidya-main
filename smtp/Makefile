CC	= gcc
CFLAGS	= -O2 -Wall -Werror -pedantic -std=gnu99
LIBS	= -lm -lpthread -no-pie

PROGRAM	= smtpcmd

$(PROGRAM): main.c smtp.a
	$(CC) $(CFLAGS) $(LIBS) -o $@ main.c smtp.a

smtp.a:
	$(CC) -c $(CFLAGS) $(LIBS) smtp.c -o smtp.a
