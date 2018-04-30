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
// int theta = 4194305; // theta, raid5 or replicates
int theta = 0;

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

// split a file into n chunks + 1 raid5 chunk
void split(std::string original_name, int n) {
  std::ifstream is(original_name, std::ifstream::binary);
  if (is) {
    // get length of file:
      is.seekg (0, is.end);
      int length = is.tellg();
      is.seekg (0);
      log_msg("file length: %d\n", length);

      int chunk_size = length / n;
      int last_chunk_size = chunk_size + length % n;
      std::string raid5_buf(last_chunk_size, 0);

      // write file chunk
      for (int i = 0; i < n; i++) {
        if (i == n - 1)
          chunk_size = last_chunk_size;
        
        std::string buf;
        buf.resize(chunk_size);
        is.read(&buf[0], chunk_size);

        // raid 5
        for (int j = 0; j < chunk_size; j++)
          raid5_buf[j] ^= buf[j];

        std::string file_name = original_name + "-part" + std::to_string(i);
        std::ofstream os(file_name, std::ofstream::binary);
        os.write(&buf[0], chunk_size);
        os.close();
      }

      // write raid-5 chunk
      std::string file_name = original_name + "-raid5";
      std::ofstream os(file_name, std::ofstream::binary);
      os.write(&raid5_buf[0], last_chunk_size);
      os.close();

      /* 
        write file_size into a file, this file should be stored in the raid-5 node
        because we need file_size to calculate the broke_chunk's size when 
        recovering using raid-5
      */
      file_name = original_name + "-size";
      std::ofstream os_size(file_name);
      os_size << length;
      os_size.close();

      if (is)
        log_msg("all characters read successfully.\n");
      else
        log_msg("error: only %d  could be read\n", is.gcount());

      is.close();
  }
}

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
