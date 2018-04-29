/*
  Big Brother File System
  Copyright (C) 2012 Joseph J. Pfeiffer, Jr., Ph.D. <pfeiffer@cs.nmsu.edu>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.

  This code is derived from function prototypes found /usr/include/fuse/fuse.h
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  His code is licensed under the LGPLv2.
  A copy of that code is included in the file fuse.h
  
  The point of this FUSE filesystem is to provide an introduction to
  FUSE.  It was my first FUSE filesystem as I got to know the
  software; hopefully, the comments in this code will help people who
  follow later to get a gentler introduction.

  This might be called a no-op filesystem:  it doesn't impose
  filesystem semantics on top of any other existing structure.  It
  simply reports the requests that come in, and passes them to an
  underlying filesystem.  The information is saved in a logfile named
  bbfs.log, in the directory from which you run bbfs.
*/
#include "config.hpp"
#include "params.hpp"
#include "log.hpp"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

// My headers
#include <string>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <array>
#include <fstream>
#include <vector>
#include <arpa/inet.h>
#include "myftpclient.hpp"
#include "myftp.hpp"

// My datas
struct node {
    in_addr_t ip;
    unsigned short port;
};

int num_storage_node;
std::vector<node> storage_nodes; //
std::string file_path; // set current open file_path (absolute)
bool is_write = false; // write or read opeartion

// My functions
static void set_file_path(char fpath[PATH_MAX]) {
    // set file path
    file_path = fpath;
}

std::string get_file_path(){
    return file_path;
}

int get_file_size(int fd) {
    struct stat buf;
    fstat(fd, &buf);
    return buf.st_size;
}

static void set_is_write() {
    is_write = true;
}

static void reset_is_write() {
    is_write = false;
}

static bool get_is_write() {
    return is_write;
}

// in_addr_t get_ip(std::string s) {
//     // ip = inet_addr("10.0.2.2")
//     in_addr_t ip = inet_addr(s.substr(0, s.find(' ')).c_str());
// }

// unsigned short get_port(std::string s) {
//     return (unsigned short) stoi(s.substr(s.find(' ')));
// }

std::string exec(std::string cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        log_msg("popen() failed!\n");
        throw std::runtime_error("popen() failed!");
    }
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            result += buffer.data();
    }
    return result;
}
