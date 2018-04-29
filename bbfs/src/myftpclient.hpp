#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>	// "struct sockaddr_in"
#include <arpa/inet.h>	// "in_addr_t"
#include <errno.h>
#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include "myftp.hpp"

void list_task(in_addr_t ip, unsigned short port)
{
	int fd;
	struct sockaddr_in addr;
	unsigned int addrlen = sizeof(struct sockaddr_in);

	fd = socket(AF_INET, SOCK_STREAM, 0);	// Create a TCP socket. AF_INET: for ipv4. SOCK_STREAM: for TCP. 0: to except raw socket

	if(fd == -1)
	{
		perror("socket()"); // fail to create socket
		exit(1);
	}

	// Below 4 lines: Set up the destination with IP address and port number.
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ip;
	addr.sin_port = htons(port);

	if( connect(fd, (struct sockaddr *) &addr, addrlen) == -1 )		// connect to the destintation
	{
		perror("connect()");
		exit(1);
	}


	struct message_s send_msg;

	// LIST_REQUEST
	memcpy(send_msg.protocol, "myftp", sizeof(send_msg.protocol));
	send_msg.type = (unsigned char)0xA1;
	send_msg.length = htonl(sizeof(send_msg));

	sendn(fd, &send_msg, sizeof(send_msg)); 
	//printf("LIST_REQUEST is sent.\n");

	// receive from network
	struct message_s recv_msg;
	int count = recvn(fd, &recv_msg, sizeof(recv_msg)); // LIST_REPLY
	int file_list_size;
	char* file_list_buf = (char*) malloc(1024); // list of files
	if(count == -1) // can not receive from network
	{
		perror("reading...");
		exit(1);
	}
	else if(count == 0) // nothing can be received.
		printf("Nothing\n");
	else if (memcmp(recv_msg.protocol, "myftp", sizeof(recv_msg.protocol)) == 0) { // protocol is "myftp"
		// handle LIST_REPLY
		if (recv_msg.type == (unsigned char) 0xA2) {
			//printf("Received LIST_REPLY.\n");
			file_list_size = ntohl(recv_msg.length) - sizeof(recv_msg);
			while ((count = recv(fd, file_list_buf, file_list_size, 0)) > 0) {
				printf("%s", file_list_buf);
			}
			printf("\n");
		}
	}

	close(fd);	// Time to shut up
}


void get_task(in_addr_t ip, unsigned short port, std::string file_name)
{
	char* file_name_cstr = (char*) file_name.c_str();
	printf("Try to get file: %s\n", file_name_cstr);

	int fd;
	struct sockaddr_in addr;
	unsigned int addrlen = sizeof(struct sockaddr_in);

	fd = socket(AF_INET, SOCK_STREAM, 0);	// Create a TCP socket. AF_INET: for ipv4. SOCK_STREAM: for TCP. 0: to except raw socket

	if(fd == -1)
	{
		perror("socket()"); // fail to create socket
		exit(1);
	}

	// Below 4 lines: Set up the destination with IP address and port number.
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ip;
	addr.sin_port = htons(port);

	if( connect(fd, (struct sockaddr *) &addr, addrlen) == -1 )		// connect to the destintation
	{
		perror("connect()");
		exit(1);
	}


	struct message_s send_msg;

	// send GET_REQUEST
	memcpy(send_msg.protocol, "myftp", sizeof(send_msg.protocol));
	send_msg.type = (unsigned char)0xB1;
	send_msg.length = htonl(sizeof(send_msg) + file_name.size() + 1);
	printf("%s name's size is %lu\n", file_name_cstr, file_name.size() + 1);
	sendn(fd, &send_msg, sizeof(send_msg));
	printf("GET_REQUEST is sent.\n");

	send(fd, file_name_cstr, file_name.size() + 1, 0); // send payload: name of the requested file

	// receive from network
	struct message_s recv_msg;
	int count = recvn(fd, &recv_msg, sizeof(recv_msg)); // GET_REPLY

	if(count == -1) // can not receive from network
	{
		perror("reading...");
		exit(1);
	}
	else if(count == 0) // nothing can be received.
		printf("Nothing\n");
	else if (memcmp(recv_msg.protocol, "myftp", sizeof(recv_msg.protocol)) == 0) { // protocol is "myftp"
		// handle GET_REPLY
		if (recv_msg.type == (unsigned char) 0xB2) {
			printf("Received GET_REPLY.\nFile exists.\n");

			// handle FILE_DATA
			struct message_s file_msg;
			count = recvn(fd, &file_msg, sizeof(file_msg));
			long long file_size;
			if (count == -1) {
				perror("reading...");
				exit(1);
			}
			else if(count == -1)
				printf("Nothing\n");
			else if (memcmp(file_msg.protocol, "myftp", sizeof(file_msg.protocol)) == 0 && file_msg.type == (unsigned char) 0xFF) {
				file_size = ntohl(file_msg.length) - sizeof(recv_msg);
				printf("file_size: %lld\n", file_size);

				// get file data
				FILE *received_file;
				int buffer_size = 1024;
				char* buffer = (char*) malloc(buffer_size);
				received_file = fopen(file_name_cstr, "wb");
				if (received_file == NULL)
				{
					fprintf(stderr, "Failed to open file: %s\n", strerror(errno));
					exit(1);
				}

				long long remaining_size = file_size;
				while(remaining_size > 0) {
					int receive_size = (buffer_size > remaining_size) ? remaining_size : buffer_size;
					int tmp = recvn(fd, buffer, receive_size); // receive file from network
					if (tmp > 0)
					{
						fwrite(buffer, sizeof(char), receive_size, received_file);
						remaining_size -= receive_size;
						// printf("Receive %d bytes\n", receive_size);
					}
					else if (tmp == 0) {
						printf("Receive 0 byte of file.\n");
					}
					else {
						printf("Cannot receive file.\n");
					}
				}

				fclose(received_file);
				free(buffer);
			}
			else {
				printf("Received unexpected type of protocol.\n");
			}

		}
		else if (recv_msg.type == (unsigned char) 0xB3) {
			printf("Received GET_REPLY.\nFile does not exist.\n");
		}
		else {
			printf("Received unexpected type of protocol.\n");
		}
	}

	close(fd);	// Time to shut up
}

void put_task(in_addr_t ip, unsigned short port, std::string file_name)
{
	char* file_name_cstr = (char*) file_name.c_str();
	printf("Try to put file: %s\n", file_name_cstr);

	if( access( file_name_cstr, F_OK ) == -1 ) {
		// file does not exist locally
		printf("File %s does not exist.\n", file_name_cstr);
		exit(1);
	}

	int fd;
	struct sockaddr_in addr;
	unsigned int addrlen = sizeof(struct sockaddr_in);

	fd = socket(AF_INET, SOCK_STREAM, 0);	// Create a TCP socket. AF_INET: for ipv4. SOCK_STREAM: for TCP. 0: to except raw socket

	if(fd == -1)
	{
		perror("socket()"); // fail to create socket
		exit(1);
	}

	// Below 4 lines: Set up the destination with IP address and port number.
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ip;
	addr.sin_port = htons(port);

	if( connect(fd, (struct sockaddr *) &addr, addrlen) == -1 )		// connect to the destintation
	{
		perror("connect()");
		exit(1);
	}

	struct message_s send_msg;
	// send PUT_REQUEST
	memcpy(send_msg.protocol, "myftp", sizeof(send_msg.protocol));
	send_msg.type = (unsigned char)0xC1;
	send_msg.length = htonl(sizeof(send_msg) + file_name.size() + 1);
	printf("%s name's size is %lu\n", file_name_cstr, file_name.size() + 1);
	sendn(fd, &send_msg, sizeof(send_msg));
	printf("PUT_REQUEST is sent.\n");

	send(fd, file_name_cstr, file_name.size() + 1, 0); // send payload: name of the file to be uploaded

	// receive from network
	struct message_s recv_msg;
	int count = recvn(fd, &recv_msg, sizeof(recv_msg)); // PUT_REPLY

	if(count == -1) // can not receive from network
	{
		perror("reading...");
		exit(1);
	}
	else if(count == 0) // nothing can be received.
		printf("Nothing\n");
	else if (memcmp(recv_msg.protocol, "myftp", sizeof(recv_msg.protocol)) == 0) { // protocol is "myftp"
		// handle PUT_REPLY
		if (recv_msg.type == (unsigned char) 0xC2) {
			printf("Received PUT_REPLY.\n");

			// send flie data
			int file_fd;
		    if((file_fd = open(file_name_cstr, O_RDONLY)) == -1) {
		      printf("cannot open file %s\n", file_name_cstr);
		      exit(1);
		    }
		    off_t file_size = lseek(file_fd, 0, SEEK_END);
		    printf("file_size: %ld\n", file_size);

			// FILE_DATA
			struct message_s file_msg;
			memcpy(file_msg.protocol, "myftp", sizeof(file_msg.protocol));
			file_msg.type = (unsigned char)0xFF;
			file_msg.length = htonl(sizeof(file_msg) + file_size);
			sendn(fd, &file_msg, sizeof(file_msg)); 
			printf("FILE_DATA is sent.\n");

			// Send file data
			lseek(file_fd, 0, SEEK_SET);
			char * buffer = (char *) malloc(sizeof(char) * 1024);
			long long offset = file_size;
			int len = 0;
			while(offset > 0) {
				int read_size = read(file_fd, buffer, 1024);
				if(read_size == -1){
				  printf("error in reading file");
				  break;
				}
				// send a buffer to server
				len = send(fd, buffer,read_size, 0);
				if(len == -1) {
					fprintf(stderr, "Failed to send file: %s\n", strerror(errno));
				  	break;
				} 
				else if(len == 0){
				  	printf("server closed connection\n");
				  	break;
				}
				offset -= read_size;
			}
			printf("put finished\n");
			close(file_fd);
			free(buffer);
		}
		else {
			printf("Received unexpected type of protocol.\n");
		}
	}

	close(fd);	// Time to shut up
}