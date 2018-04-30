# My-Distributed-File-System (MYFS)
A Distributed File System using FUSE, extended from BBFS (Big Brother File System) (https://www.cs.nmsu.edu/~pfeiffer/fuse-tutorial/)

### Prerequisite

Make sure you have **libfuse** and **pkg-config** installed first:

```bash
sudo apt-get install libfuse-dev
sudo apt-get install pkg-config
```

Enter the ```bbfs``` folder, configure and make the system:

```bash
./configure
make
```

On client node, put a file ```storagenodes.conf``` into the ```example``` folder, stating the datanodes configs:

```
3
10.0.2.1 15436
10.0.2.2 15436
10.0.2.3 15436
```

The first line is the number of datanodes n, while the following n lines is the ip and port of the n nodes.

### Mount

Enter the ```example``` folder, run the ```build.sh``` script to mount  ```rootdir``` to```mountdir``` . You  can modity the content of the script to set your own mountdir, rootdir, as well as theta (in bytes) for determining large and small file.

```Shell
make
../src/bbfs rootdir/ mountdir/ 4194305
```

### Set datanodes

Copy the whole repo to all your datanodes, go into ```/bbfs/src``` folder, run the ```server.sh``` script to start listening TCP request from client

### Test

Generate one large file (100MB) and one small file (4MB) for test:

```bash
dd if=/dev/zero of=100MB.dat  bs=100M  count=1
dd if=/dev/zero of=4MB.dat  bs=4M  count=1
```

- **Writing large ﬁles**: ```cp 100MB.dat mountdir```
- **Storing small ﬁles**: ```cp 4MB.dat mountdir```
- **Reading large ﬁles from n-1 nodes**: Go to any of the datanode, use ```Ctrl + C``` to stop the server, and use ```cp mountdir/100MB.dat new.dat``` to read the large file, and use ```diff 100MB.dat new.dat``` to test if two files are the same.
- **Benchmarking**: Use the ```write.sh``` and the ```read.sh``` scripts to test the sequential read/write performance of MYFS via IOZone. (Make sure to install IOZone first: ```sudo apt-get install iozone3```)