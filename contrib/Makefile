
#LINKOPT	=	/usr/local/lib/libdbmalloc.a
#CFLAGS	=	-g -DUSE_DBMALLOC

CFLAGS	=	-I..

SHPOBJ	=	../shpopen.o ../dbfopen.o 

default:	all

all:	shpdxf shpproj

clean:  
	rm -f shpdxf shpproj


shpdxf:	shpdxf.c $(SHPOBJ)
	$(CC) $(CFLAGS) shpdxf.c ${SHPOBJ} $(LINKOPT) -o shpdxf

shpproj: shpproj.c $(SHPOBJ)
	$(CC) $(CFLAGS) shpproj.c  $(SHPOBJ) -lm -lproj $(LINKOPT) -o shpproj
	
testproj: 
	tests/shpproj.sh

	  