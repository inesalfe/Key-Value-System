CC=gcc -std=c99 -g -pthread -I./include

all: KVS-LocalServer app KVS-AuthServer

KVS-LocalServer: appList.o hash.o groupList.o KVS-LocalServer.c
	$(CC) KVS-LocalServer.c -o KVS-LocalServer appList.o groupList.o hash.o

KVS-AuthServer: KVS-AuthServer.c hash.o
	$(CC) KVS-AuthServer.c -o KVS-AuthServer hash.o

app: KVS-lib.o app.c
	$(CC) app.c -o app KVS-lib.o

KVS-lib.o: KVS-lib.c KVS-lib.h
	$(CC) -c KVS-lib.c

hash.o: hash.c hash.h
	$(CC) -c hash.c

groupList.o: groupList.c groupList.h
	$(CC) -c groupList.c

appList.o: appList.c appList.h
	$(CC) -c appList.c

clean:
	rm -f *.o *~
