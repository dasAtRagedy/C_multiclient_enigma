all: client server
client: client.c
	gcc -o client client.c

server: server.c enigma.c
	gcc -o server server.c enigma.c