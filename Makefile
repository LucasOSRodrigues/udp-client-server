all: client server

client:
	gcc cliente_udp.c -o client

server:
	gcc servidor_udp.c -o server

clean:
	rm -f client server