CC = cc
TARGET = mvlf
SOURCE = mvlf.c
DESTDIR = /usr/bin
CFLAGS= -Wall -g

all:
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)

install:
	cp $(TARGET) $(DESTDIR)
