#ifndef MYFTP_H_
#define MYFTP_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // "struct sockaddr_in"
#include <arpa/inet.h>  // "in_addr_t"
#include <errno.h>
#include <string>

struct message_s {
	unsigned char protocol[5]; /* protocol string (5 bytes) */
	unsigned char type; /* type (1 byte) */
	unsigned int length; /* length (header + payload) (4 bytes) */
} __attribute__((packed));

int sendn(int sd, struct message_s *buf, int buf_len);

int recvn(int sd, struct message_s *buf, int buf_len);

int sendn(int sd, char *buf, int buf_len);

int recvn(int sd, char *buf, int buf_len);

std::string get_relative_path(std::string filename);

#endif // MYFTP_H_
