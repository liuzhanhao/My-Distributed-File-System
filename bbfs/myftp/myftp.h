#ifndef MYFTP_H_
#define MYFTP_H_

struct message_s {
	unsigned char protocol[5]; /* protocol string (5 bytes) */
	unsigned char type; /* type (1 byte) */
	unsigned int length; /* length (header + payload) (4 bytes) */
} __attribute__((packed));

int sendn(int sd, struct message_s *buf, int buf_len);

int recvn(int sd, struct message_s *buf, int buf_len);

int sendn(int sd, char *buf, int buf_len);

int recvn(int sd, char *buf, int buf_len);

#endif // MYFTP_H_
