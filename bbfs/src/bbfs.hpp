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
#include <mutex>
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
int theta; // theta, raid5 or replicates
int cur_num_of_read = 0;
std::mutex mtx; // you can use std::lock_guard if you want to be exception safe
int my_fd;


//  All the paths I see are relative to the root of the mounted
//  filesystem.  In order to get to the underlying filesystem, I need to
//  have the mountpoint.  I'll save it away early on in main(), and then
//  whenever I need a path for something I'll call this to construct
//  it.
static void bb_fullpath(char fpath[PATH_MAX], const char *path)
{
    strcpy(fpath, BB_DATA->rootdir);
    strncat(fpath, path, PATH_MAX); // ridiculously long paths will
                    // break here

    log_msg("    bb_fullpath:  rootdir = \"%s\", path = \"%s\", fpath = \"%s\"\n",
        BB_DATA->rootdir, path, fpath);
}


// My functions
static void set_file_path(char fpath[PATH_MAX]) {
    // set file path
    file_path = fpath;
}

std::string get_file_path(){
    return file_path;
}

int get_local_file_size(int fd) {
    struct stat buf;
    fstat(fd, &buf);
    return buf.st_size;
}

int get_real_file_size(const char *path) {
    struct stat s;
    if( stat(path, &s) == 0) {
        if(s.st_mode & S_IFDIR) {
          //it's a directory
          log_msg("%s is a directory\n", path);
          return s.st_size;
        }
        else if (s.st_mode & S_IFREG) {
          //it's a file
          log_msg("%s is a file\n", path);
          // non-exist
          if (s.st_size <= 0)
            return 0;
          // remote file (real_file_size < 10^12)
          else if (s.st_size < 12) {
            std::ifstream is_size(path);
            if (!is_size.good()) {
                log_msg("[get_real_file_size] Can't open file: %s\n", path);
                return -1;
            }
            // get length of file:
            std::string line;
            std::getline(is_size, line);
            int file_size = stoi(line);
            is_size.close();
            return file_size;
          }
          else {
            return s.st_size;
          }
        }
        else
          return 0;
    }
    else
      return 0;
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

      // file_name = original_name + "-size";
      // std::ofstream os_size(file_name);
      // os_size << length;
      // os_size.close();

      if (is)
        log_msg("all characters read successfully.\n");
      else
        log_msg("error: only %d  could be read\n", is.gcount());

      is.close();
  }
}

// n is the number of datanodes that with real data chunk, not raid5 chunk
// so n = num_of_datanode - 1
void merge(std::string original_name, std::string new_name, int n) {
  std::ofstream os(new_name, std::ofstream::binary);
  if (os) {
      // read file chunk
      for (int i = 0; i < n; i++) {
        std::string file_name = original_name + "-part" + std::to_string(i);
        std::ifstream is(file_name, std::ifstream::binary);

        // get length of file:
        is.seekg (0, is.end);
        int length = is.tellg();
        is.seekg (0);
        log_msg("length of file %d: %d\n", i, length);
        
        std::string buf;
        buf.resize(length);
        is.read(&buf[0], length);

        os.write(&buf[0], length);
        is.close();
      }
      os.close();
  }
}

// broke_chunk: the index of the chunk which is broken, which will be skipped in recover
void recover(std::string original_name, std::string new_name, int n, int broke_chunk, int file_size) {
  std::ofstream os(new_name, std::ofstream::binary);

  // read raid-5 file
  std::string raid5_file = original_name + "-raid5";
  std::ifstream is_raid5(raid5_file, std::ifstream::binary);
  int chunk_size = file_size / n;
  int last_chunk_size = chunk_size + file_size % n;
  log_msg("file_size: %d\n", file_size);
  log_msg("chunk_size: %d\n", chunk_size);
  log_msg("last_chunk_size: %d\n", last_chunk_size);

  // read raid-5 file into buf
  std::string raid5_buf;
  raid5_buf.resize(last_chunk_size);
  is_raid5.read(&raid5_buf[0], last_chunk_size);
  is_raid5.close();

  if (os) {
      // read file chunk for 0 to broke_chunk - 1
      for (int i = 0; i < broke_chunk; i++) {
        std::string file_name = original_name + "-part" + std::to_string(i);
        std::ifstream is(file_name, std::ifstream::binary);

        int length = (i == n-1) ? last_chunk_size : chunk_size;
        log_msg("%s  length: %d\n", file_name.c_str(), length);
        
        std::string buf;
        buf.resize(length);
        is.read(&buf[0], length);

        // raid 5
        for (int j = 0; j < length; j++)
          raid5_buf[j] ^= buf[j];

        os.write(&buf[0], length);
        is.close();
      }
      // vector of string for broke_chunk + 1 to n-1
      std::vector<std::string> v;
      for (int i = broke_chunk + 1; i < n; i++) {
        std::string file_name = original_name + "-part" + std::to_string(i);
        std::ifstream is(file_name, std::ifstream::binary);

        int length = (i == n-1) ? last_chunk_size : chunk_size;
        log_msg("%s  length: %d\n", file_name.c_str(), length);
        
        std::string buf;
        buf.resize(length);
        is.read(&buf[0], length);
        v.push_back(buf);

        // raid 5
        for (int j = 0; j < length; j++)
          raid5_buf[j] ^= buf[j];

        is.close();
      }
      // write broke chunk
      // check if broke_chunk's size equals raid5_buf's size
      if (broke_chunk != n - 1){
        raid5_buf = raid5_buf.substr(0, chunk_size);
        os.write(&raid5_buf[0], chunk_size);
      }
      else {
        os.write(&raid5_buf[0], last_chunk_size);
      }
      // write the remaining chunks (for broke_chunk+1 to n-1)
      for (int i = broke_chunk + 1; i < n; i++) {
        if (i < n - 1) {
          os.write(&v[i-broke_chunk-1][0], chunk_size);
        }
        else {
          os.write(&v[i-broke_chunk-1][0], last_chunk_size);
        }
      }
      os.close();
  }
}
