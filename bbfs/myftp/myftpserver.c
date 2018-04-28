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
#include <sys/wait.h>
#include <iostream>
#include <vector>
#include <pthread.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "myftp.h"

pthread_mutex_t mutex;

std::string list_data() {
	// ref: https://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
	DIR *dir;
	struct dirent *entry;
	std::string file_list = "";
	if ((dir = opendir ("data")) != NULL) {
	  /* print all the files and directories within directory */
	  while ((entry = readdir (dir)) != NULL) {
	  	if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue; // do not display . or ..
	  	}
	  	file_list += (std::string)entry->d_name + "\n";
	  }
	  std::cout << entry << std::endl;
	  closedir (dir);
	} else {
	  /* could not open directory */
	  perror ("could not open directory data/");
	  exit(1);
	}
	return file_list;
}

// LIST_REQUEST handler
void list_handler(int client_fd) {
	std::string file_list = list_data(); // get list of files in server's directory data/
	const char* file_list_cstr = file_list.c_str();
	//printf("%s\n", file_list_cstr);

	struct message_s send_msg;

	// LIST_REPLY
	memcpy(send_msg.protocol, "myftp", sizeof(send_msg.protocol));
	send_msg.type = (unsigned char)0xA2;
	send_msg.length = htonl(sizeof(send_msg) + file_list.size() + 1);

	sendn(client_fd, &send_msg, sizeof(send_msg)); 
	printf("LIST_REPLY is sent.\n");

	send(client_fd, file_list_cstr, file_list.size() + 1, 0);
	printf("list of files is sent.\n");

	close(client_fd);	// Time to shut up
}

// GET_REQUEST handler
void get_handler(int client_fd, int file_name_length) {
	char* file_name = (char*) malloc(file_name_length);
	recv(client_fd, file_name, file_name_length, 0);
	printf("length of file name: %d\n", file_name_length);
	printf("Name of file requested by client: %s\n", file_name);
	char* file_name_expanded = (char*) malloc(file_name_length + 5);
	strcpy(file_name_expanded, "data/");
	strcat(file_name_expanded, file_name);

	if( access( file_name_expanded, F_OK ) != -1 ) {
		// file exists
		printf("File %s exists.\n", file_name_expanded);

		// GET_REPLY
		struct message_s send_msg;
		memcpy(send_msg.protocol, "myftp", sizeof(send_msg.protocol));
		send_msg.type = (unsigned char)0xB2;
		send_msg.length = htonl(sizeof(send_msg));
		sendn(client_fd, &send_msg, sizeof(send_msg)); 
		printf("GET_REPLY is sent.\n");

		// send file data
		int file_fd;
	    if((file_fd = open(file_name_expanded, O_RDONLY)) == -1) {
	      printf("cannot open file %s\n", file_name_expanded);
	      exit(1);
	    }
	    off_t file_size = lseek(file_fd, 0, SEEK_END);
	    printf("file_size: %ld\n", file_size);

	    // FILE_DATA
		struct message_s file_msg;
		memcpy(file_msg.protocol, "myftp", sizeof(file_msg.protocol));
		file_msg.type = (unsigned char)0xFF;
		file_msg.length = htonl(sizeof(file_msg) + file_size);
		sendn(client_fd, &file_msg, sizeof(file_msg)); 
		printf("FILE_DATA is sent.\n");

		// Send file data
		lseek(file_fd, 0, SEEK_SET);
		char * buffer = (char *) malloc(sizeof(char) * 1024);
		long long remaining_size = file_size;
		int len = 0;
		while(remaining_size > 0) {
			//printf("remaining_size is %lld\n", remaining_size);
			int read_size = read(file_fd, buffer, 1024);
			if(read_size == -1){
			  printf("error in reading file");
			  break;
			}
			// send a buffer to server
			len = sendn(client_fd, buffer, read_size);
			if(len == -1) {
				fprintf(stderr, "Failed to send file: %s\n", strerror(errno));
			  	break;
			} 
			else if(len == 0){
			  	printf("server closed connection\n");
			  	break;
			}
			remaining_size -= read_size;
		}
		printf("Done.\n");
		close(file_fd);
		free(buffer);
	} 
	else {
		// file doesn't exist
		printf("File %s does not exist.\n", file_name_expanded);

		// GET_REPLY
		struct message_s send_msg;
		memcpy(send_msg.protocol, "myftp", sizeof(send_msg.protocol));
		send_msg.type = (unsigned char)0xB3;
		send_msg.length = htonl(sizeof(send_msg));
		sendn(client_fd, &send_msg, sizeof(send_msg)); 
		printf("GET_REPLY is sent.\n");
	} 

	close(client_fd);
}

void put_handler(int client_fd, int file_name_length) {
	char* file_name = (char*) malloc(file_name_length);
	recv(client_fd, file_name, file_name_length, 0);
	printf("Flie name's length: %d\n", file_name_length);
	printf("Name of file requested by client: %s\n", file_name);
	char* file_name_expanded = (char*) malloc(file_name_length + 5);
	strcpy(file_name_expanded, "data/");
	strcat(file_name_expanded, file_name);

	// PUT_REPLY
	struct message_s send_msg;
	memcpy(send_msg.protocol, "myftp", sizeof(send_msg.protocol));
	send_msg.type = (unsigned char)0xC2;
	send_msg.length = htonl(sizeof(send_msg));
	sendn(client_fd, &send_msg, sizeof(send_msg)); 
	printf("GET_REPLY is sent.\n");

	// handle FILE_DATA
	struct message_s file_msg;
	int count = recvn(client_fd, &file_msg, sizeof(file_msg));
	int file_size;
	if (count == -1) {
		perror("reading...");
		exit(1);
	}
	else if(count == -1)
		printf("Nothing\n");
	else if (memcmp(file_msg.protocol, "myftp", sizeof(file_msg.protocol)) == 0 && file_msg.type == (unsigned char) 0xFF) {
		file_size = ntohl(file_msg.length) - sizeof(file_msg);
		printf("%d\n", file_size);

		// get file data
		FILE *received_file;
		int buffer_size = 1024;
		char* buffer = (char*) malloc(buffer_size);
		received_file = fopen(file_name_expanded, "wb");
		if (received_file == NULL)
		{
			fprintf(stderr, "Failed to open file: %s\n", strerror(errno));
			exit(1);
		}

		free(file_name_expanded);
		free(file_name);

		long long remaining_size = file_size;
		while(remaining_size > 0) {
			int receive_size = (buffer_size > remaining_size) ? remaining_size : buffer_size;
			int tmp = recvn(client_fd, buffer, receive_size); // receive file from network
			if (tmp > 0)
			{
				fwrite(buffer, sizeof(char), receive_size, received_file);
				remaining_size -= receive_size;
				//printf("Receive %d bytes\n", receive_size);
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

void *pthread_prog(void *args) {
	int count, client_fd = *(int *)args;
	struct message_s recv_msg;

	printf("I am thread.\n");

	// Receive from network
	count = recvn(client_fd, &recv_msg, sizeof(recv_msg));

	if(count == -1) // can not receive from network
	{
		perror("reading...");
		exit(1);
	}
	else if(count == 0) // nothing can be received.
		printf("Nothing\n");
	else if (memcmp(recv_msg.protocol, "myftp", sizeof(recv_msg.protocol)) == 0) { // protocol is "myftp"
		// handle LIST_REQUEST
		if (recv_msg.type == (unsigned char) 0xA1) {
			printf("Received LIST_REQUEST.\n");
			list_handler(client_fd);
		}
		// handle GET_REQUEST
		else if (recv_msg.type == (unsigned char) 0xB1) {
			printf("Received GET_REQUEST\n");
			int file_name_length = ntohl(recv_msg.length) - sizeof(recv_msg);
			get_handler(client_fd, file_name_length);
		}
		// handle PUT_REQUEST
		else if (recv_msg.type == (unsigned char) 0xC1) {
			printf("Received PUT_REQUEST\n");
			int file_name_length = ntohl(recv_msg.length) - sizeof(recv_msg);
			put_handler(client_fd, file_name_length);
		}
	}

	close(client_fd); 
	pthread_exit(NULL);
}

void main_loop(unsigned short port)
{
	int fd, accept_fd;
	struct sockaddr_in addr, tmp_addr;
	unsigned int addrlen = sizeof(struct sockaddr_in);

	fd = socket(AF_INET, SOCK_STREAM, 0); // Create a TCP Socket

	if(fd == -1)
	{
		perror("socket()");
		exit(1);
	}

	// set the port to be reusable
	long val = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(long)) == -1) {
		perror("setsockopt");
		exit(1);
	}

	// 4 lines below: setting up the port for the listening socket
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	// After the setup has been done, invoke bind()
	if(bind(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
	{
		perror("bind()");
		exit(1);
	}

	// Switch to listen mode by invoking listen()
	if( listen(fd, 1024) == -1 )
	{
		perror("listen()");
		exit(1);
	}

	printf("[To stop the server: press Ctrl + C]\n");

	//int client_count = 0;
	//std::vector <pthread_t*> thread;
	//std::vector <int> thread_arg;
	pthread_mutex_init(&mutex, NULL);
	while(1) {
		printf("About to accept fd.\n");

		// Accept one client
		if( (accept_fd = accept(fd, (struct sockaddr *) &tmp_addr, &addrlen)) == -1)
		{
			perror("accept()");
			exit(1);
		}

		//client_count++;
		//printf("[Main thread:] Connection count = %d\n", client_count);
		//thread.push_back(new pthread_t());
		//thread_arg.push_back(accept_fd);

		if(pthread_create(new pthread_t(), NULL, pthread_prog, &accept_fd)){
			perror("pthread_create()");
			exit(-1);
		}

		printf("I am main.\n");
	}
}


int main(int argc, char **argv)
{
	unsigned short port;

	if(argc != 2)
	{
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit(1);
	}

	port = atoi(argv[1]);

	DIR* dir = opendir("data");
	if (dir)
	{
	    /* Directory data/ exists. */
	    closedir(dir);
	}
	else if (ENOENT == errno)
	{
	    /* Directory data/ does not exist, create it. */
	    const int dir_err = mkdir("data", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if (-1 == dir_err)
		{
		    printf("Error creating directory data/\n");
		    exit(1);
		}
	}
	else
	{
	    printf("Error checking directory data/\n");
	    exit(1);
	}

	main_loop(port);

	return 0;
}
