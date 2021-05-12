CC=gcc

KVS-LocalServer: appList.o hash.o groupList.o KVS-LocalServer.c
	$(CC) KVS-LocalServer.c -o KVS-LocalServer appList.o groupList.o hash.o

app: KVS-lib.o app.c
	$(CC) app.c -o app KVS-lib.o

KVS-lib.o: KVS-lib.c KVS-lib.h
	$(CC) -c KVS-lib.c

main_hash: hash.o main_hash.c
	$(CC) main_hash.c -o main_hash hash.o

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