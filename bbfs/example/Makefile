all:
	make clean
	mkdir -p mountdir
	mkdir -p rootdir
	echo "bogus file" > rootdir/bogus.txt
	../src/bbfs rootdir/ mountdir/

distdir:
	cp Makefile $(distdir)

mostlyclean clean distclean mainainer-clean:
	fusermount -u mountdir
	rm -r mountdir rootdir
