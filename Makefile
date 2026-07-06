TERMINAL = konsole # ajustar para o terminal do sistema operacional
all: client server

client:
	gcc cliente_udp.c -o client

server:
	gcc servidor_udp.c -o server

clean:
	rm -f client server

# 	Cria um novo terminal para execução em paralelo
run-server: server
	$(TERMINAL) -e "./server" &

run-client: client
	$(TERMINAL) -e "./client" &


run: client server run-server run-client