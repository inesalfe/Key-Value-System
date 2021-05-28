CC=gcc

all: KVS-LocalServer app1 app2 app3 main_group main_app KVS-AuthServer

KVS-LocalServer: appList.o hash.o groupList.o KVS-LocalServer.c
	$(CC) KVS-LocalServer.c -o KVS-LocalServer appList.o groupList.o hash.o

KVS-AuthServer: KVS-AuthServer.c hash.o
	$(CC) KVS-AuthServer.c -o KVS-AuthServer hash.o

app1: KVS-lib.o app1.c
	$(CC) app1.c -o app1 KVS-lib.o

app2: KVS-lib.o app2.c
	$(CC) app2.c -o app2 KVS-lib.o

app3: app3.c
	$(CC) app3.c -o app3

KVS-lib.o: KVS-lib.c KVS-lib.h
	$(CC) -c KVS-lib.c

hash.o: hash.c hash.h
	$(CC) -c hash.c

main_group: appList.o hash.o groupList.o main_group.c
	$(CC) main_group.c -o main_group appList.o hash.o groupList.o

groupList.o: groupList.c groupList.h
	$(CC) -c groupList.c

main_app: appList.o main_app.c
	$(CC) main_app.c -o main_app appList.o

appList.o: appList.c appList.h
	$(CC) -c appList.c

clean:
	rm -f *.o core a.out proj *~