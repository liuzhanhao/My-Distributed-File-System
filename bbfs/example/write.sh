echo "hello" > mountdir/io.out
iozone -+n -i0 -s 100m -r 4k -e -c -w -f mountdir/io.out
