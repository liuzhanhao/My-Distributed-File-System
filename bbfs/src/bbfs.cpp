#include "bbfs.hpp"

///////////////////////////////////////////////////////////
//
// Prototypes for all these functions, and the C-style comments,
// come from /usr/include/fuse.h
//
/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int bb_getattr(const char *path, struct stat *statbuf)
{
    int retstat;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_getattr(path=\"%s\", statbuf=0x%08x)\n",
      path, statbuf);
    bb_fullpath(fpath, path);

    retstat = log_syscall("lstat", lstat(fpath, statbuf), 0);

    statbuf->st_size = (off_t) get_real_file_size(fpath);
    
    log_stat(statbuf);
    
    return retstat;
}

/** Read the target of a symbolic link
 *
 * The buffer should be filled with a null terminated string.  The
 * buffer size argument includes the space for the terminating
 * null character.  If the linkname is too long to fit in the
 * buffer, it should be truncated.  The return value should be 0
 * for success.
 */
// Note the system readlink() will truncate and lose the terminating
// null.  So, the size passed to to the system readlink() must be one
// less than the size passed to bb_readlink()
// bb_readlink() code by Bernardo F Costa (thanks!)
int bb_readlink(const char *path, char *link, size_t size)
{
    int retstat;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_readlink(path=\"%s\", link=\"%s\", size=%d)\n",
      path, link, size);
    bb_fullpath(fpath, path);

    retstat = log_syscall("readlink", readlink(fpath, link, size - 1), 0);
    if (retstat >= 0) {
    link[retstat] = '\0';
    retstat = 0;
    log_msg("    link=\"%s\"\n", link);
    }
    
    return retstat;
}

/** Create a file node
 *
 * There is no create() operation, mknod() will be called for
 * creation of all non-directory, non-symlink nodes.
 */
// shouldn't that comment be "if" there is no.... ?
int bb_mknod(const char *path, mode_t mode, dev_t dev)
{
    int retstat;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_mknod(path=\"%s\", mode=0%3o, dev=%lld)\n",
      path, mode, dev);
    bb_fullpath(fpath, path);
    
    // On Linux this could just be 'mknod(path, mode, dev)' but this
    // tries to be be more portable by honoring the quote in the Linux
    // mknod man page stating the only portable use of mknod() is to
    // make a fifo, but saying it should never actually be used for
    // that.
    if (S_ISREG(mode)) {
    retstat = log_syscall("open", open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode), 0);
    if (retstat >= 0)
        retstat = log_syscall("close", close(retstat), 0);
    } else
    if (S_ISFIFO(mode))
        retstat = log_syscall("mkfifo", mkfifo(fpath, mode), 0);
    else
        retstat = log_syscall("mknod", mknod(fpath, mode, dev), 0);
    
    return retstat;
}

/** Create a directory */
int bb_mkdir(const char *path, mode_t mode)
{
    char fpath[PATH_MAX];
    
    log_msg("\nbb_mkdir(path=\"%s\", mode=0%3o)\n",
        path, mode);
    bb_fullpath(fpath, path);

    return log_syscall("mkdir", mkdir(fpath, mode), 0);
}

/** Remove a file */
int bb_unlink(const char *path)
{
    char fpath[PATH_MAX];
    
    log_msg("bb_unlink(path=\"%s\")\n",
        path);
    bb_fullpath(fpath, path);

    return log_syscall("unlink", unlink(fpath), 0);
}

/** Remove a directory */
int bb_rmdir(const char *path)
{
    char fpath[PATH_MAX];
    
    log_msg("bb_rmdir(path=\"%s\")\n",
        path);
    bb_fullpath(fpath, path);

    return log_syscall("rmdir", rmdir(fpath), 0);
}

/** Create a symbolic link */
// The parameters here are a little bit confusing, but do correspond
// to the symlink() system call.  The 'path' is where the link points,
// while the 'link' is the link itself.  So we need to leave the path
// unaltered, but insert the link into the mounted directory.
int bb_symlink(const char *path, const char *link)
{
    char flink[PATH_MAX];
    
    log_msg("\nbb_symlink(path=\"%s\", link=\"%s\")\n",
        path, link);
    bb_fullpath(flink, link);

    return log_syscall("symlink", symlink(path, flink), 0);
}

/** Rename a file */
// both path and newpath are fs-relative
int bb_rename(const char *path, const char *newpath)
{
    char fpath[PATH_MAX];
    char fnewpath[PATH_MAX];
    
    log_msg("\nbb_rename(fpath=\"%s\", newpath=\"%s\")\n",
        path, newpath);
    bb_fullpath(fpath, path);
    bb_fullpath(fnewpath, newpath);

    return log_syscall("rename", rename(fpath, fnewpath), 0);
}

/** Create a hard link to a file */
int bb_link(const char *path, const char *newpath)
{
    char fpath[PATH_MAX], fnewpath[PATH_MAX];
    
    log_msg("\nbb_link(path=\"%s\", newpath=\"%s\")\n",
        path, newpath);
    bb_fullpath(fpath, path);
    bb_fullpath(fnewpath, newpath);

    return log_syscall("link", link(fpath, fnewpath), 0);
}

/** Change the permission bits of a file */
int bb_chmod(const char *path, mode_t mode)
{
    char fpath[PATH_MAX];
    
    log_msg("\nbb_chmod(fpath=\"%s\", mode=0%03o)\n",
        path, mode);
    bb_fullpath(fpath, path);

    return log_syscall("chmod", chmod(fpath, mode), 0);
}

/** Change the owner and group of a file */
int bb_chown(const char *path, uid_t uid, gid_t gid)
  
{
    char fpath[PATH_MAX];
    
    log_msg("\nbb_chown(path=\"%s\", uid=%d, gid=%d)\n",
        path, uid, gid);
    bb_fullpath(fpath, path);

    return log_syscall("chown", chown(fpath, uid, gid), 0);
}

/** Change the size of a file */
int bb_truncate(const char *path, off_t newsize)
{
    char fpath[PATH_MAX];
    
    log_msg("\nbb_truncate(path=\"%s\", newsize=%lld)\n",
        path, newsize);
    bb_fullpath(fpath, path);

    return log_syscall("truncate", truncate(fpath, newsize), 0);
}

/** Change the access and/or modification times of a file */
/* note -- I'll want to change this as soon as 2.6 is in debian testing */
int bb_utime(const char *path, struct utimbuf *ubuf)
{
    char fpath[PATH_MAX];
    
    log_msg("\nbb_utime(path=\"%s\", ubuf=0x%08x)\n",
        path, ubuf);
    bb_fullpath(fpath, path);

    return log_syscall("utime", utime(fpath, ubuf), 0);
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int bb_open(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    int fd;
    char fpath[PATH_MAX];

    bb_fullpath(fpath, path);
    set_file_path(fpath);
    
    log_msg("\nbb_open(path\"%s\", file_path\"%s\", fi=0x%08x)\n",
        path, get_file_path().c_str(), fi);
    
    // if the open call succeeds, my retstat is the file descriptor,
    // else it's -errno.  I'm making sure that in that case the saved
    // file descriptor is exactly -1.
    fd = log_syscall("open", open(fpath, fi->flags), 0);
    if (fd < 0)
    retstat = log_error("open");
    
    fi->fh = fd;

    mtx.lock();
    my_fd = fd;
    mtx.unlock();

    log_fi(fi);
    
    return retstat;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
// I don't fully understand the documentation above -- it doesn't
// match the documentation for the read() system call which says it
// can return with anything up to the amount of data requested. nor
// with the fusexmp code which returns the amount of data also
// returned by read.
int bb_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;

    // my functions
    cur_num_of_read++;
    // load data from remote first
    if (cur_num_of_read == 1) {
        log_msg("\n------------------------------------------\n");
        log_msg("Pulling file from remote storage nodes...\n\n");
        std::string file_name = get_file_path();
        int file_size = get_real_file_size(file_name.c_str());
        // Delete file with filename
        retstat = log_syscall("close", close(fi->fh), 0);
        if(remove(file_name.c_str()) != 0)
            log_msg("Can't remove file %s!!!!\n", file_name.c_str());

        bool read_success = true;
        if (file_size > theta) { // Large file: read real file from all storage nodes and merge
            std::vector<int> broke_chunk;
            for (int i = 0; i < num_storage_node - 1; i++) {
                std::string chunk_name = file_name + "-part" + std::to_string(i);
                int status = get_task(storage_nodes[i].ip, storage_nodes[i].port, chunk_name);
                if (status < 0) {
                    broke_chunk.push_back(i);
                    if (broke_chunk.size() > 1) {
                        read_success = false;
                        break;
                    }
                }
            }
            // clean the filename-part* files
            if (!read_success) {
                for (int i = 0; i < broke_chunk[1]; i++) {
                    if (i != broke_chunk[0]) {
                        std::string chunk_name = file_name + "-part" + std::to_string(i);
                        if(remove(chunk_name.c_str()) != 0)
                            log_msg("Can't remove file %s!!!!\n", chunk_name.c_str());
                    }
                }
            }
            else { // read_success, merge the files
                // No broke_chunk, just merge
                if (broke_chunk.size() == 0) {
                    // merge file_name-part* into file_name
                    merge(file_name, file_name, num_storage_node-1);
                    // remove file_name-part*
                    for (int i = 0; i < num_storage_node-1; i++) {
                        std::string chunk_name = file_name + "-part" + std::to_string(i);
                        if(remove(chunk_name.c_str()) != 0)
                            log_msg("Can't remove file %s!!!!\n", chunk_name.c_str());
                    }
                }
                else { // One broke_chunk, read raid-5 chunk to recover the chunk and merge
                    std::string raid5_file = file_name + "-raid5";
                    int status = get_task(storage_nodes[num_storage_node-1].ip, storage_nodes[num_storage_node-1].port, raid5_file);
                    if (status < 0) {
                        // Cannot get raid5 chunk either, fail, remove file_name-part*
                        read_success = false;
                    }
                    else { // Get raid-5 chunk success, recover 
                        recover(file_name, file_name, num_storage_node-1, broke_chunk[0], file_size);
                        // remove file_name-raid5
                        if(remove(raid5_file.c_str()) != 0)
                                log_msg("Can't remove file %s!!!!\n", raid5_file.c_str());
                    }
                    // remove file_name-part*
                    for (int i = 0; i < num_storage_node-1; i++) {
                        if (i != broke_chunk[0]) {
                            std::string chunk_name = file_name + "-part" + std::to_string(i);
                            if(remove(chunk_name.c_str()) != 0)
                                log_msg("Can't remove file %s!!!!\n", chunk_name.c_str());
                        }
                    }
                }
            }
        }
        else { // Small file, read from any available node
            read_success = false;
            for (int i = 0; i < storage_nodes.size(); i++) {
                int status = get_task(storage_nodes[i].ip, storage_nodes[i].port, file_name);
                if (status > 0) {
                    read_success = true;
                    break;
                }
            }
        }
        if (!read_success) {
            log_msg("[bb_read] get_task failed!!!\n");
            exit(-1);
        }

        int fd = log_syscall("open", open(file_name.c_str(), fi->flags), 0);
        if (fd < 0)
            retstat = log_error("[bb_read] open");
        
        mtx.lock();
        my_fd = fd;
        mtx.unlock();

        log_msg("\n\nFinished pulling file from remote storage nodes...");
        log_msg("\n------------------------------------------\n");
    }
    log_msg("cur_num_of_read: %d, my_fd: %d\n", cur_num_of_read, my_fd);
    log_msg("\nbb_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
        path, buf, size, offset, fi);
    // no need to get fpath on this one, since I work from fi->fh not the path
    log_fi(fi);

    return log_syscall("pread", pread(my_fd, buf, size, offset), 0);
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
// As  with read(), the documentation above is inconsistent with the
// documentation for the write() system call.
int bb_write(const char *path, const char *buf, size_t size, off_t offset,
         struct fuse_file_info *fi)
{
    int retstat = 0;
    set_is_write();
    
    log_msg("\nbb_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
        path, buf, size, offset, fi
        );
    // no need to get fpath on this one, since I work from fi->fh not the path
    log_fi(fi);

    return log_syscall("pwrite", pwrite(fi->fh, buf, size, offset), 0);
}

/** Get file system statistics
 *
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 *
 * Replaced 'struct statfs' parameter with 'struct statvfs' in
 * version 2.5
 */
int bb_statfs(const char *path, struct statvfs *statv)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_statfs(path=\"%s\", statv=0x%08x)\n",
        path, statv);
    bb_fullpath(fpath, path);
    
    // get stats for underlying filesystem
    retstat = log_syscall("statvfs", statvfs(fpath, statv), 0);
    
    log_statvfs(statv);
    
    return retstat;
}

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().  This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.  It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 */
// this is a no-op in BBFS.  It just logs the call and returns success
int bb_flush(const char *path, struct fuse_file_info *fi)
{
    log_msg("\nbb_flush(path=\"%s\", fi=0x%08x)\n", path, fi);
    // no need to get fpath on this one, since I work from fi->fh not the path
    log_fi(fi);
    
    return 0;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int bb_release(const char *path, struct fuse_file_info *fi)
{
    cur_num_of_read = 0;

    int retstat = 0;
    int size = get_local_file_size(fi->fh);

    log_msg("\nbb_release(path=\"%s\", file_size=%d, fi=0x%08x)\n",
      path, size, fi);
    log_fi(fi);

    std::string file_name = get_file_path();
    // Check write or read
    log_msg("\n------------------------------------------\n");
    if (get_is_write()) {
        log_msg("Write operation!!!!!!!!!!!!!\n");
        reset_is_write();

        // rootdir: /home/csci4430/jasper/My-Distributed-File-System/bbfs/example/rootdir
        bool greater_than_theta;
        // > theta
        if (size > theta) {
            greater_than_theta = true;
            log_msg("Spliting file into %d chunks + 1 raid5 chunk...\n", storage_nodes.size()-1);
            log_msg("Sending file to %d storage nodes...\n\n", storage_nodes.size());
            split(file_name, num_storage_node - 1);

            for (int i = 0; i < num_storage_node; i++) {
                std::string chunk_name = file_name + "-part" + std::to_string(i);
                if (i == num_storage_node - 1)
                    chunk_name = file_name + "-raid5";
                int status = put_task(storage_nodes[i].ip, storage_nodes[i].port, chunk_name);
                if (status < 0)
                    log_msg("put_task at node %d failed!!!\n", i);
            }
            // Delete all files in rootdir
            retstat = log_syscall("close", close(fi->fh), 0);
            if(remove(file_name.c_str()) != 0)
                log_msg("Can't remove file %s!!!!\n", file_name.c_str());
            for (int i = 0; i < num_storage_node; i++) {
                std::string chunk_name = file_name + "-part" + std::to_string(i);
                if (i == num_storage_node - 1)
                    chunk_name = file_name + "-raid5";
                if(remove(chunk_name.c_str()) != 0)
                    log_msg("Can't remove file %s!!!!\n", chunk_name.c_str());
            }
            // Create file with file_name, which store only the file size
            std::ofstream os_size(file_name);
            os_size << size;
            os_size.close();
        }
        // < theta
        else {
            greater_than_theta = false;
            log_msg("Sending file to %d storage nodes...\n\n", storage_nodes.size());
            for (auto node : storage_nodes) {
                int status = put_task(node.ip, node.port, file_name);
                if (status < 0)
                    log_msg("put_task failed!!!\n");
            }
        }
        // Delete all files in rootdir
        retstat = log_syscall("close", close(fi->fh), 0);
        if(remove(file_name.c_str()) != 0)
            log_msg("Can't remove file %s!!!!\n", file_name.c_str());
        if (greater_than_theta) {
            for (int i = 0; i < num_storage_node; i++) {
                std::string chunk_name = file_name + "-part" + std::to_string(i);
                if (i == num_storage_node - 1)
                    chunk_name = file_name + "-raid5";
                if(remove(chunk_name.c_str()) != 0)
                    log_msg("Can't remove file %s!!!!\n", chunk_name.c_str());
            }
        }
        log_msg("Finished sending file to %d storage nodes...\n", storage_nodes.size());
    }
    else {
        log_msg("Read operation!!!!!!!!!!!!!\n");
        // read file size from filename
        size = get_local_file_size(my_fd);
        log_msg("file_size: %d\n", size);

        // Delete file with filename
        retstat = log_syscall("close", close(my_fd), 0);
        if(remove(file_name.c_str()) != 0)
            log_msg("Can't remove file %s!!!!\n", file_name.c_str()); 
    }

    // Create file with file_name, which store only the file size
    std::ofstream os_size(file_name);
    os_size << size;
    os_size.close();

    log_msg("------------------------------------------\n");

    // We need to close the file.  Had we allocated any resources
    // (buffers etc) we'd need to free them here as well.
    return retstat;
}

/** Synchronize file contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data.
 *
 * Changed in version 2.2
 */
int bb_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
    log_msg("\nbb_fsync(path=\"%s\", datasync=%d, fi=0x%08x)\n",
        path, datasync, fi);
    log_fi(fi);
    
    // some unix-like systems (notably freebsd) don't have a datasync call
#ifdef HAVE_FDATASYNC
    if (datasync)
    return log_syscall("fdatasync", fdatasync(fi->fh), 0);
    else
#endif  
    return log_syscall("fsync", fsync(fi->fh), 0);
}

#ifdef HAVE_SYS_XATTR_H
/** Note that my implementations of the various xattr functions use
    the 'l-' versions of the functions (eg bb_setxattr() calls
    lsetxattr() not setxattr(), etc).  This is because it appears any
    symbolic links are resolved before the actual call takes place, so
    I only need to use the system-provided calls that don't follow
    them */

/** Set extended attributes */
int bb_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
    char fpath[PATH_MAX];
    
    log_msg("\nbb_setxattr(path=\"%s\", name=\"%s\", value=\"%s\", size=%d, flags=0x%08x)\n",
        path, name, value, size, flags);
    bb_fullpath(fpath, path);

    return log_syscall("lsetxattr", lsetxattr(fpath, name, value, size, flags), 0);
}

/** Get extended attributes */
int bb_getxattr(const char *path, const char *name, char *value, size_t size)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_getxattr(path = \"%s\", name = \"%s\", value = 0x%08x, size = %d)\n",
        path, name, value, size);
    bb_fullpath(fpath, path);

    retstat = log_syscall("lgetxattr", lgetxattr(fpath, name, value, size), 0);
    if (retstat >= 0)
    log_msg("    value = \"%s\"\n", value);
    
    return retstat;
}

/** List extended attributes */
int bb_listxattr(const char *path, char *list, size_t size)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    char *ptr;
    
    log_msg("\nbb_listxattr(path=\"%s\", list=0x%08x, size=%d)\n",
        path, list, size
        );
    bb_fullpath(fpath, path);

    retstat = log_syscall("llistxattr", llistxattr(fpath, list, size), 0);
    if (retstat >= 0) {
    log_msg("    returned attributes (length %d):\n", retstat);
    if (list != NULL)
        for (ptr = list; ptr < list + retstat; ptr += strlen(ptr)+1)
        log_msg("    \"%s\"\n", ptr);
    else
        log_msg("    (null)\n");
    }
    
    return retstat;
}

/** Remove extended attributes */
int bb_removexattr(const char *path, const char *name)
{
    char fpath[PATH_MAX];
    
    log_msg("\nbb_removexattr(path=\"%s\", name=\"%s\")\n",
        path, name);
    bb_fullpath(fpath, path);

    return log_syscall("lremovexattr", lremovexattr(fpath, name), 0);
}
#endif

/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int bb_opendir(const char *path, struct fuse_file_info *fi)
{
    DIR *dp;
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_opendir(path=\"%s\", fi=0x%08x)\n",
      path, fi);
    bb_fullpath(fpath, path);

    // since opendir returns a pointer, takes some custom handling of
    // return status.
    dp = opendir(fpath);
    log_msg("    opendir returned 0x%p\n", dp);
    if (dp == NULL)
    retstat = log_error("bb_opendir opendir");
    
    fi->fh = (intptr_t) dp;

    mtx.lock();
    my_fd = fi->fh;
    mtx.unlock();
    
    log_fi(fi);
    
    return retstat;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */

int bb_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
           struct fuse_file_info *fi)
{
    int retstat = 0;
    DIR *dp;
    struct dirent *de;
    
    log_msg("\nbb_readdir(path=\"%s\", buf=0x%08x, filler=0x%08x, offset=%lld, fi=0x%08x)\n",
        path, buf, filler, offset, fi);
    // once again, no need for fullpath -- but note that I need to cast fi->fh
    dp = (DIR *) (uintptr_t) fi->fh;

    // Every directory contains at least two entries: . and ..  If my
    // first call to the system readdir() returns NULL I've got an
    // error; near as I can tell, that's the only condition under
    // which I can get an error from readdir()
    de = readdir(dp);
    log_msg("    readdir returned 0x%p\n", de);
    if (de == 0) {
    retstat = log_error("bb_readdir readdir");
    return retstat;
    }

    // This will copy the entire directory into the buffer.  The loop exits
    // when either the system readdir() returns NULL, or filler()
    // returns something non-zero.  The first case just means I've
    // read the whole directory; the second means the buffer is full.
    do {
    log_msg("calling filler with name %s\n", de->d_name);
    if (filler(buf, de->d_name, NULL, 0) != 0) {
        log_msg("    ERROR bb_readdir filler:  buffer full");
        return -ENOMEM;
    }
    } while ((de = readdir(dp)) != NULL);
    
    log_fi(fi);
    
    return retstat;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int bb_releasedir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_releasedir(path=\"%s\", fi=0x%08x)\n",
        path, fi);
    log_fi(fi);
    
    closedir((DIR *) (uintptr_t) fi->fh);
    
    return retstat;
}

/** Synchronize directory contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data
 *
 * Introduced in version 2.3
 */
// when exactly is this called?  when a user calls fsync and it
// happens to be a directory? ??? >>> I need to implement this...
int bb_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_fsyncdir(path=\"%s\", datasync=%d, fi=0x%08x)\n",
        path, datasync, fi);
    log_fi(fi);
    
    return retstat;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
// Undocumented but extraordinarily useful fact:  the fuse_context is
// set up before this function is called, and
// fuse_get_context()->private_data returns the user_data passed to
// fuse_main().  Really seems like either it should be a third
// parameter coming in here, or else the fact should be documented
// (and this might as well return void, as it did in older versions of
// FUSE).
void *bb_init(struct fuse_conn_info *conn)
{
    log_msg("\nbb_init()\n");
    
    log_conn(conn);
    log_fuse_context(fuse_get_context());
    
    return BB_DATA;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void bb_destroy(void *userdata)
{
    log_msg("\nbb_destroy(userdata=0x%08x)\n", userdata);
}

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 */
int bb_access(const char *path, int mask)
{
    int retstat = 0;
    char fpath[PATH_MAX];
   
    log_msg("\nbb_access(path=\"%s\", mask=0%o)\n",
        path, mask);
    bb_fullpath(fpath, path);
    
    retstat = access(fpath, mask);
    
    if (retstat < 0)
    retstat = log_error("bb_access access");
    
    return retstat;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
// Not implemented.  I had a version that used creat() to create and
// open the file, which it turned out opened the file write-only.

/**
 * Change the size of an open file
 *
 * This method is called instead of the truncate() method if the
 * truncation was invoked from an ftruncate() system call.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the truncate() method will be
 * called instead.
 *
 * Introduced in version 2.5
 */
int bb_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_ftruncate(path=\"%s\", offset=%lld, fi=0x%08x)\n",
        path, offset, fi);
    log_fi(fi);
    
    retstat = ftruncate(fi->fh, offset);
    if (retstat < 0)
    retstat = log_error("bb_ftruncate ftruncate");
    
    return retstat;
}

/**
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Introduced in version 2.5
 */
int bb_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_fgetattr(path=\"%s\", statbuf=0x%08x, fi=0x%08x)\n",
        path, statbuf, fi);
    log_fi(fi);

    // On FreeBSD, trying to do anything with the mountpoint ends up
    // opening it, and then using the FD for an fgetattr.  So in the
    // special case of a path of "/", I need to do a getattr on the
    // underlying root directory instead of doing the fgetattr().
    if (!strcmp(path, "/"))
    return bb_getattr(path, statbuf);
    
    retstat = fstat(fi->fh, statbuf);
    if (retstat < 0)
    retstat = log_error("bb_fgetattr fstat");
    
    log_stat(statbuf);
    
    return retstat;
}

static const struct fuse_operations bb_oper = {
  bb_getattr,
  bb_readlink,
  /*getdir :*/ NULL,
  bb_mknod,
  bb_mkdir,
  bb_unlink,
  bb_rmdir,
  bb_symlink,
  bb_rename,
  bb_link,
  bb_chmod,
  bb_chown,
  bb_truncate,
  bb_utime,
  bb_open,
  bb_read,
  bb_write,
  bb_statfs,
  bb_flush,
  bb_release,
  bb_fsync,
  
#ifdef HAVE_SYS_XATTR_H
  bb_setxattr,
  bb_getxattr,
  bb_listxattr,
  bb_removexattr,
#endif
  
  bb_opendir,
  bb_readdir,
  bb_releasedir,
  bb_fsyncdir,
  bb_init,
  bb_destroy,
  bb_access,
  /*ftruncate*/ NULL,
  /*fgetattr*/ NULL,
};

void bb_usage()
{
    fprintf(stderr, "usage:  bbfs [FUSE and mount options] rootDir mountPoint theta\n");
    abort();
}

int main(int argc, char *argv[])
{
    int fuse_stat;
    struct bb_state *bb_data;

    // bbfs doesn't do any access checking on its own (the comment
    // blocks in fuse.h mention some of the functions that need
    // accesses checked -- but note there are other functions, like
    // chown(), that also need checking!).  Since running bbfs as root
    // will therefore open Metrodome-sized holes in the system
    // security, we'll check if root is trying to mount the filesystem
    // and refuse if it is.  The somewhat smaller hole of an ordinary
    // user doing it with the allow_other flag is still there because
    // I don't want to parse the options string.
    if ((getuid() == 0) || (geteuid() == 0)) {
        fprintf(stderr, "Running BBFS as root opens unnacceptable security holes\n");
        return 1;
    }

    // See which version of fuse we're running
    fprintf(stderr, "Fuse library version %d.%d\n", FUSE_MAJOR_VERSION, FUSE_MINOR_VERSION);

    // Check datanodes file exist
    std::string line;
    std::ifstream infile("storagenodes.conf");
    if (infile.fail()) {
        std::cout << "storagenodes.conf not exist, please create it first" << std::endl;
        abort();
    }
    std::getline(infile, line);
    num_storage_node = stoi(line);
    fprintf(stderr, "num_storage_node: %d\n", num_storage_node);
    int count = 0;
    while (std::getline(infile, line)) {
        count++;
        in_addr_t ip = inet_addr(line.substr(0, line.find(' ')).c_str());
        if (ip == -1) {
            std::cout << "Invalid ip: " << line << std::endl;
            abort();
        }
        unsigned short port = stoi(line.substr(line.find(' ')));
        if (port < 0 || port > 65535) {
            std::cout << "Invalid ip: " << line << std::endl;
            abort();
        }
        storage_nodes.push_back({ip, port});
        if (count == num_storage_node)
            break;
    }
    infile.close();
    if (count < num_storage_node) {
        std::cout << "num_storage_node is " << num_storage_node << ", but number of lines in config is " << count << std::endl;
        abort();
    }

    std::cout << "there are total " << storage_nodes.size() << " storage nodes (ip port): " << std::endl;
    for (auto node : storage_nodes)
        std::cout << inet_ntoa(*(struct in_addr *)&node.ip) << " " << node.port << std::endl;
    
    // Perform some sanity checking on the command line:  make sure
    // there are enough arguments, and that neither of the last two
    // start with a hyphen (this will break if you actually have a
    // rootpoint or mountpoint whose name starts with a hyphen, but so
    // will a zillion other programs)
    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
        bb_usage();

    bb_data = (struct bb_state *) malloc(sizeof(struct bb_state));
    if (bb_data == NULL) {
        perror("main calloc");
        abort();
    }

    // Pull the rootdir out of the argument list and save it in my
    // internal data
    theta = std::atoi(argv[argc-1]);
    bb_data->rootdir = realpath(argv[argc-3], NULL);
    argv[argc-3] = argv[argc-2];
    argv[argc-1] = NULL;
    argv[argc-2] = NULL;
    argc -= 2;
    
    bb_data->logfile = log_open();
    
    // turn over control to fuse
    fprintf(stderr, "about to call fuse_main\n");
    fuse_stat = fuse_main(argc, argv, &bb_oper, bb_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    
    return fuse_stat;
}
