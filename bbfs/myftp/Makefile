CFLAGS=-lpthread -Wall

EXE=myftpclient myftpserver

all: ${EXE}

clean:
	rm -f ${EXE}

myftpclient: myftpclient.cpp myftpclient.hpp 
	g++ -o myftpclient myftpclient.cpp myftp.cpp $(CFLAGS)

myftpserver: myftpserver.cpp myftpserver.hpp
	g++ -o myftpserver myftpserver.cpp myftp.cpp $(CFLAGS)
