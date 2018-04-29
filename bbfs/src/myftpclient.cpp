#include "myftpclient.hpp"

int main(int argc, char **argv)
{
	in_addr_t ip;
	unsigned short port;


	if (argc != 4 && argc != 5)
	{
		fprintf(stderr, "Usage: %s [IP address] [port]\n", argv[0]);
		exit(1);
	}

	if ( (ip = inet_addr(argv[1])) == -1 )
	{
		perror("inet_addr()");
		exit(1);
	}

	port = atoi(argv[2]);

	if (strcmp(argv[3], "list") == 0) {
		//printf("command: list\n");
		if (argc != 4) {
			fprintf(stderr, "Usage of list command: %s [IP address] [port] list\n", argv[0]);
			exit(1);
		}
		else
			list_task(ip, port); // issue command list
	}
	else if (strcmp(argv[3], "get") == 0) {
		printf("command: get\n");
		if (argc != 5) {
			fprintf(stderr, "Usage of get command: %s [IP address] [port] get [file]\n", argv[0]);
			exit(1);
		}
		else
			get_task(ip, port, argv[4]); // issue command get
	}
	else if (strcmp(argv[3], "put") == 0) {
		printf("command: put\n");
		if (argc != 5) {
			fprintf(stderr, "Usage of get command: %s [IP address] [port] put [file]\n", argv[0]);
			exit(1);
		}
		else
			put_task(ip, port, argv[4]); // issue command put
	}
	else
		printf("Usage of command: %s <server ip addr> <server port> <list|get|put> <file>\n", argv[0]);

	return 0;
}
