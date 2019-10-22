CC = gcc
CFLAGS = -Wall -pedantic
LOGS = client.log supervisor.log

.PHONY: all clean test

all: client server supervisor

client: client.o utils.o
	$(CC) $(CFLAGS) $^ -o $@

server: server.o utils.o
	$(CC) $(CFLAGS) $^ -o $@ -lpthread

supervisor: supervisor.o utils.o hash.o
	$(CC) $(CFLAGS) $^ -o $@ -lpthread -lm

client.o: client.c utils.h
server.o: server.c utils.h
supervisor.o: supervisor.c supervisor.h utils.h
utils.o: utils.c utils.h
hash.o: hash.c hash.h

clean:
	@echo "Removing garbage"
	-rm -f *.o
	-rm -f *.log

test:
	@echo "Running Test..."
	./test.sh $(LOGS)
	@echo "Printing Measurements"
	./misura.sh $(LOGS)