
default:	all

all:	shpcreate shpadd shpdump dbfcreate dbfadd dbfdump shputils

shpopen.o:	shpopen.c shapefil.h
	$(CC) $(CFLAGS) -c shpopen.c

dbfopen.o:	shpopen.c shapefil.h
	$(CC) $(CFLAGS) -c dbfopen.c

shpcreate:	shpcreate.c shpopen.o
	$(CC) $(CFLAGS) shpcreate.c shpopen.o -o shpcreate

shpadd:		shpadd.c shpopen.o
	$(CC) $(CFLAGS) shpadd.c shpopen.o -o shpadd

shpdump:	shpdump.c shpopen.o
	$(CC) $(CFLAGS) shpdump.c shpopen.o -o shpdump

dbfcreate:	dbfcreate.c dbfopen.o
	$(CC) $(CFLAGS) dbfcreate.c dbfopen.o -o dbfcreate

dbfadd:		dbfadd.c dbfopen.o
	$(CC) $(CFLAGS) dbfadd.c dbfopen.o -o dbfadd

dbfdump:	dbfdump.c dbfopen.o
	$(CC) $(CFLAGS) dbfdump.c dbfopen.o -o dbfdump

shputils:	shputils.c shpopen.o dbfopen.o
	$(CC) $(CFLAGS) shputils.c shpopen.o dbfopen.o -o shputils

clean:
	rm *.o dbfdump dbfcreate dbfadd shpdump shpcreate shpadd
