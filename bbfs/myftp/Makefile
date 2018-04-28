CC=gcc
CFLAGS=-lpthread -Wall

EXE=myftpclient myftpserver

all: ${EXE}

clean:
	rm -f ${EXE}

myftpclient: myftpclient.c 
	g++ -o myftpclient myftpclient.c myftp.c $(CFLAGS)

myftpserver: myftpserver.c
	perl -i -pe 's/dirent64/dirent/g' myftpserver.c
	perl -i -pe 's/readdir64/readdir/g' myftpserver.c
	g++ -o myftpserver myftpserver.c myftp.c $(CFLAGS)