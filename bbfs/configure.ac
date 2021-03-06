#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([fuse-tutorial], [2017-10-03], [joseph@pfeifferfamily.net])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/bbfs.cpp])
AC_CONFIG_SRCDIR([src/myftpclient.cpp])
AC_CONFIG_SRCDIR([src/myftpserver.cpp])
AC_CONFIG_HEADERS([src/config.hpp])

# Checks for programs.
# AC_PROG_CC
# Jasper's config
CXXFLAGS="$CXXFLAGS -std=c++0x"
AC_PROG_CXX

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h sys/statvfs.h unistd.h utime.h sys/xattr.h arpa/inet.h netinet/in.h sys/socket.h sys/time.h])

# Check for FUSE development environment
PKG_CHECK_MODULES(FUSE, fuse)

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_STRUCT_ST_BLOCKS
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([ftruncate mkdir mkfifo realpath rmdir strerror utime memset socket])

# Not all systems that support FUSE also support fdatasync (notably freebsd)
AC_CHECK_FUNCS([fdatasync])

AC_CONFIG_FILES([Makefile html/Makefile src/Makefile])
AC_OUTPUT
