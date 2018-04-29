bin_PROGRAMS = bbfs myftpclient myftpserver
bbfs_SOURCES = bbfs.cpp log.cpp log.hpp  params.hpp myftpclient.hpp
myftpclient_SOURCES = myftpclient.cpp myftpclient.hpp myftp.hpp myftp.cpp
myftpserver_SOURCES = myftpserver.cpp myftpserver.hpp myftp.hpp myftp.cpp
AM_CXXFLAGS = @FUSE_CFLAGS@
LDADD = @FUSE_LIBS@
