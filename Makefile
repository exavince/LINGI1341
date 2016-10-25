main: client.o server.o packet_interface.o
	@echo "server et client\n"
	@gcc -o server server.o packet_interface.o -lz -L/
	@gcc -o client clien.o packet_interface.o -lz -L/
	
client.o: client.c packet_interface.h
	@echo "client.o\n"
	@gcc -c client.o client.c packet_interface.h
	
server.o: server.c packet_interface.h
	@echo "server.o\n"
	@gcc -c server.o server.c packet_interface.h
	
packet_interface.o: packet_interface.c packet_interface.h
	@echo "pakcet_interface.o\n"
	@gcc -c packet_interface.o packet_interface.c -lz

clean:
	rm *.o server client
