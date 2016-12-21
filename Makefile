CC = cc
TARGET = egglog
SOURCE = egglog.c
DESTDIR = /usr/bin
CFLAGS= -Wall -g 

all:
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)

install:
	cp $(TARGET) $(DESTDIR)
