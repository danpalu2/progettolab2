CC = gcc
CFLAGS = -Wall -pthread

all: archivio client1 
	chmod u+x server.py
	chmod u+x client2.py

archivio: xerrori.o archivio.o prodcons.o hashtable.o threads.o
	$(CC) $(CFLAGS) -o archivio xerrori.o archivio.o prodcons.o hashtable.o threads.o

client1: xerrori.o client1.o prodcons.o hashtable.o threads.o
	$(CC) $(CFLAGS) -o client1 xerrori.o client1.o prodcons.o hashtable.o threads.o

xerrori.o: xerrori.c
	$(CC) $(CFLAGS) -c xerrori.c

archivio.o: archivio.c
	$(CC) $(CFLAGS) -c archivio.c

client1.o: client1.c
	$(CC) $(CFLAGS) -c client1.c

prodcons.o: prodcons.c
	$(CC) $(CFLAGS) -c prodcons.c

hashtable.o: hashtable.c
	$(CC) $(CFLAGS) -c hashtable.c

threads.o: threads.c
	$(CC) $(CFLAGS) -c threads.c

clean:
	rm -f *.o archivio client1
