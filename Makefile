
#LINKOPT	=	/usr/local/lib/libdbmalloc.a
#CFLAGS	=	-g -DUSE_DBMALLOC

default:	all

all:	shpcreate shpadd shpdump dbfcreate dbfadd dbfdump shptest

shpopen.o:	shpopen.c shapefil.h
	$(CC) $(CFLAGS) -c shpopen.c

dbfopen.o:	shpopen.c shapefil.h
	$(CC) $(CFLAGS) -c dbfopen.c

shpcreate:	shpcreate.c shpopen.o
	$(CC) $(CFLAGS) shpcreate.c shpopen.o $(LINKOPT) -o shpcreate

shpadd:		shpadd.c shpopen.o
	$(CC) $(CFLAGS) shpadd.c shpopen.o $(LINKOPT) -o shpadd

shpdump:	shpdump.c shpopen.o
	$(CC) $(CFLAGS) shpdump.c shpopen.o $(LINKOPT) -o shpdump

dbfcreate:	dbfcreate.c dbfopen.o
	$(CC) $(CFLAGS) dbfcreate.c dbfopen.o $(LINKOPT) -o dbfcreate

dbfadd:		dbfadd.c dbfopen.o
	$(CC) $(CFLAGS) dbfadd.c dbfopen.o $(LINKOPT) -o dbfadd

dbfdump:	dbfdump.c dbfopen.o
	$(CC) $(CFLAGS) dbfdump.c dbfopen.o $(LINKOPT) -o dbfdump

shptest:	shptest.c shpopen.o
	$(CC) $(CFLAGS) shptest.c shpopen.o $(LINKOPT) -o shptest

shputils:	shputils.c shpopen.o dbfopen.o
	$(CC) $(CFLAGS) shputils.c shpopen.o dbfopen.o $(LINKOPT) -o shputils

clean:
	rm -f *.o dbfdump dbfcreate dbfadd shpdump shpcreate shpadd shputils
	rm -f shptest

test:	test2

#
#	Note this stream only works if example data is accessable.
#
test1:
	@./stream1.sh > s1.out
	@if test "`diff s1.out stream1.out`" = '' ; then \
	    echo "******* Stream 1 Succeeded *********"; \
	    rm s1.out; \
	else \
	    echo "******* Stream 1 Failed *********"; \
	    diff s1.out stream1.out; \
	fi

test2:
	@./stream2.sh > s2.out
	@if test "`diff s2.out stream2.out`" = '' ; then \
	    echo "******* Stream 2 Succeeded *********"; \
	    rm s2.out; \
	    rm test*.s??; \
	else \
	    echo "******* Stream 2 Failed *********"; \
	    diff s2.out stream2.out; \
	fi
