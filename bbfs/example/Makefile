all:
	make clean
	mkdir -p mountdir
	mkdir -p rootdir
	echo "bogus file" > rootdir/bogus.txt

distdir:
	cp Makefile $(distdir)

mostlyclean clean distclean mainainer-clean:
	fusermount -u mountdir
	rm -r mountdir rootdir bbfs.log
