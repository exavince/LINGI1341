all: sender receiver

sender: src/sender.o
	gcc -o src/sender src/sender.o -lz

sender.o: src/sender.c src/packet_implement.h
	gcc -c src/sender.c -lz

receiver: src/receiver.o
	gcc -o src/receiver src/receiver.o -lz

receiver.o: src/receiver.c src/packet_implement.h
	gcc -c src/receiver.c -lz


##############################################################################

.PHONY: clean

clean:
	rm -rf src/*o
	rm -rf src/sender
	rm -rf src/receiver


###############################################################################

.PHONY: tests

test:
