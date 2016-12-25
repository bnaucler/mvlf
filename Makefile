CC = cc
TARGET = renlog
SOURCE = renlog.c
DESTDIR = /usr/bin
CFLAGS= -Wall -g

all:
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)

install:
	cp $(TARGET) $(DESTDIR)
