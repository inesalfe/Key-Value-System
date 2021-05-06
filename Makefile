CC=gcc

app: KVS-lib.o app.c
	$(CC) app.c -o app KVS-lib.o

KVS-LocalServer: KVS-LocalServer.c
	$(CC) KVS-LocalServer.c -o KVS-LocalServer

KVS-lib.o: KVS-lib.c KVS-lib.h
	$(CC) -c KVS-lib.c

clean:
	rm -f *.o core a.out proj *~