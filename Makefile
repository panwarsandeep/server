# Makefile
# Copyright (C) 2015 Sandeep Panwar.
# For conditions of distribution and use, see copyright notice in socket_daemon.c

CC=gcc

ODIR=obj
LDIR =../lib

LIBS=-lpthread -levent

DEPS = rand_string.h workqueue.h

OBJ = socket_daemon.o rand_string.o workqueue.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< 

socketdaemon: $(OBJ)
	gcc -o $@ $^ $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o socketdaemon 
