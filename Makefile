CC=gcc -g -pthread -I./include

all: bin/KVS-LocalServer bin/app1 bin/app2 bin/app3 bin/app bin/main_group bin/main_app bin/KVS-AuthServer

bin/KVS-LocalServer: bin/appList.o bin/hash.o bin/groupList.o KVS-LocalServer.c
	$(CC) KVS-LocalServer.c -o bin/KVS-LocalServer bin/appList.o bin/groupList.o bin/hash.o

bin/KVS-AuthServer: KVS-AuthServer.c bin/hash.o
	$(CC) KVS-AuthServer.c -o bin/KVS-AuthServer bin/hash.o

bin/app: bin/KVS-lib.o app.c
	$(CC) app.c -o bin/app bin/KVS-lib.o

bin/app1: bin/KVS-lib.o TestFiles/app1.c
	$(CC) TestFiles/app1.c -o bin/app1 bin/KVS-lib.o

bin/app2: bin/KVS-lib.o TestFiles/app2.c
	$(CC) TestFiles/app2.c -o bin/app2 bin/KVS-lib.o

bin/app3: src/app3.c
	$(CC) TestFiles/app3.c -o bin/app3

bin/KVS-lib.o: src/KVS-lib.c src/KVS-lib.h
	$(CC) -c src/KVS-lib.c

bin/hash.o: src/hash.c src/hash.h
	$(CC) -c src/hash.c

bin/main_group: bin/appList.o bin/hash.o bin/groupList.o TestFiles/main_group.c
	$(CC) TestFiles/main_group.c -o bin/main_group bin/appList.o bin/hash.o bin/groupList.o

bin/groupList.o: src/groupList.c src/groupList.h
	$(CC) -c src/groupList.c

bin/main_app: bin/appList.o TestFiles/main_app.c
	$(CC) TestFiles/main_app.c -o bin/main_app bin/appList.o

bin/appList.o: src/appList.c src/appList.h
	$(CC) -c src/appList.c

clean:
	rm -f bin/* *~