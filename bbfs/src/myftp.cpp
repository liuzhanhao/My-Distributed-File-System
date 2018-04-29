#include "myftp.hpp"

int sendn(int sd, struct message_s *buf, int buf_len) {
    int n_left = buf_len;
    int n;
    while (n_left > 0) {
        if ((n = send(sd, buf + (buf_len - n_left), n_left, 0)) < 0) {
            if (errno == EINTR)
                n = 0;
            else
                return -1;
        } else if (n == 0) {
            return 0;
        }
        n_left -= n;
    }
    return buf_len;
}

int recvn(int sd, struct message_s *buf, int buf_len) {
    int n_left = buf_len;
    int n;
    while (n_left > 0) {
        if ((n = recv(sd, buf + (buf_len - n_left), n_left, 0)) < 0) {
            if (errno == EINTR)
                n = 0;
            else
                return -1;
        } else if (n == 0) {
            return 0;
        }
        n_left -= n;
    }
    return buf_len;
}

int sendn(int sd, char *buf, int buf_len) {
    int n_left = buf_len;
    int n;
    while (n_left > 0) {
        if ((n = send(sd, buf + (buf_len - n_left), n_left, 0)) < 0) {
            if (errno == EINTR)
                n = 0;
            else
                return -1;
        } else if (n == 0) {
            return 0;
        }
        n_left -= n;
    }
    return buf_len;
}

int recvn(int sd, char *buf, int buf_len) {
    int n_left = buf_len;
    int n;
    while (n_left > 0) {
        if ((n = recv(sd, buf + (buf_len - n_left), n_left, 0)) < 0) {
            if (errno == EINTR)
                n = 0;
            else
                return -1;
        } else if (n == 0) {
            return 0;
        }
        n_left -= n;
    }
    return buf_len;
}

std::string get_relative_path(char * filename) {
    std::string real_file_name = filename;
    std::size_t found = real_file_name.find_last_of("/");
    return real_file_name.substr(found + 1);
}
