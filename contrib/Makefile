
#LINKOPT	=	/usr/local/lib/libdbmalloc.a
#CFLAGS	=	-g -DUSE_DBMALLOC

CFLAGS	=	-I.. -DPROJ4 -DDEBUG -DDEBUG2 

#CFLAGS  =	-I.. -DPROJ4

SHPOBJ	=	../shpopen.o ../dbfopen.o 

SHPGOBJ =	../shpopen.o ../dbfopen.o shpgeo.o

GEOOBJ	=	shpgeo.o -lm -lproj

default:	all

all:	shpgeo.o shpdxf shpproj dbfinfo shpcentrd shpdata shpwkb

clean:  
	rm -f shpdxf shpproj dbfinfo shpcentrd shpdata shpwkb

shpgeo.o:  shpgeo.c	
	$(CC) $(CFLAGS) -c shpgeo.c  -o shpgeo.o

shpdxf:	shpdxf.c $(SHPOBJ)
	$(CC) $(CFLAGS) shpdxf.c ${SHPOBJ} $(LINKOPT) -o shpdxf

shpcentrd:	shpcentrd.c $(SHPGOBJ)
	$(CC) $(CFLAGS) shpcentrd.c ${SHPOBJ} $(LINKOPT) $(GEOOBJ) -o shpcentrd
	
shpdata:	shpdata.c $(SHPGOBJ)
	$(CC) $(CFLAGS) shpdata.c ${SHPOBJ} $(LINKOPT) $(GEOOBJ) -o shpdata

shpwkb:	shpwkb.c $(SHPGOBJ)
	$(CC) $(CFLAGS) shpwkb.c ${SHPOBJ} $(LINKOPT) $(GEOOBJ) -o shpwkb

shpproj: shpproj.c $(SHPOBJ)
	$(CC) $(CFLAGS) shpproj.c  $(SHPOBJ) $(GEOOBJ) -lm -lproj $(LINKOPT) -o shpproj

dbfinfo: dbfinfo.c $(SHPOBJ)
	$(CC) $(CFLAGS) dbfinfo.c $(SHPOBJ) $(LINKOPT) -o dbfinfo
		
check:	testproj
		
testproj: 
	tests/shpproj.sh

	  
