all: servers clients

servers: hangman_server
clients: hangman_client

# server_c_tcp
hangman_server: hangman_server.o libserver.a libserver_tcp.a libutil.a liblog.a
	gcc -lm -o hangman_server hangman_server.o -L. -lserver -lserver_tcp -lutil -llog
hangman_server.o: hangman_server.c
	gcc -O -c hangman_server.c

# client_c_tcp
hangman_client: hangman_client.o libclient.a libclient_tcp.a libutil.a liblog.a
	gcc -lm -o hangman_client hangman_client.o -L. -lclient -lclient_tcp -lutil -llog
hangman_client.o: hangman_client.c

# libserver
server.o: server.c server.h
	gcc -O -c server.c
libserver.a: server.o
	ar rcs libserver.a server.o

# libclient
client.o: client.c client.h
	gcc -O -c client.c
libclient.a: client.o
	ar rcs libclient.a client.o

# libserver_tcp
server_tcp.o: server_tcp.c server_tcp.h
	gcc -O -c server_tcp.c
libserver_tcp.a: server_tcp.o
	ar rcs libserver_tcp.a server_tcp.o

# libclient_tcp
client_tcp.o: client_tcp.c client_tcp.h
	gcc -O -c client_tcp.c
libclient_tcp.a: client_tcp.o
	ar rcs libclient_tcp.a client_tcp.o

# libutil
util.o: util.c util.h
	gcc -O -c util.c
libutil.a: util.o
	ar rcs libutil.a util.o

# liblog
log.o: third_party/log.c third_party/log.h
	gcc -O -c third_party/log.c
liblog.a: log.o
	ar rcs liblog.a log.o

clean:
	rm -f *.o *.a hangman_server hangman_client

.PHONY: all clean
