CC = gcc
CFLAGS = -Wall
LIBS = -lpthread -lrt

all: orion eternal

orion: orion.c arena.h
	$(CC) $(CFLAGS) orion.c -o orion $(LIBS)

eternal: eternal.c arena.h
	$(CC) $(CFLAGS) eternal.c -o eternal $(LIBS)

clean:
	rm -f orion eternal users.dat

clear_ipc:
	@echo "Clearing Shared Memory and Semaphores..."
	ipcrm -M 1234 || true
	ipcrm -S 5678 || true
	@echo "IPC Cleared!"
