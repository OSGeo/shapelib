
#LINKOPT	=	/usr/local/lib/libdbmalloc.a
#CFLAGS	=	-g -DUSE_DBMALLOC

CFLAGS	=	-I.. -DPROJ4 -DDEBUG -DDEBUG2 

#CFLAGS  =	-I.. -DPROJ4

SHPOBJ	=	../shpopen.o ../dbfopen.o 

SHPGOBJ =	../shpopen.o ../dbfopen.o shpgeo.o

GEOOBJ	=	./shpgeo.o -lm -lproj

default:	all

all:	shpdxf shpproj dbfinfo shpcentrd shpdata shpwkb dbfinfo dbfcat shpinfo

clean:  
	rm -f shpdxf shpproj dbfinfo shpcentrd shpdata shpwkb dbfcat dbfinfo shpinfo

shpgeo.o:	shpgeo.c shpgeo.h
	$(CC) $(CFLAGS) -c shpgeo.c

shpdxf:		shpdxf.c $(SHPOBJ)
	$(CC) $(CFLAGS) shpdxf.c ${SHPOBJ} $(LINKOPT) -o shpdxf

shpcentrd:	shpcentrd.c $(SHPGOBJ)
	$(CC) $(CFLAGS) shpcentrd.c ${SHPOBJ} $(LINKOPT) $(GEOOBJ) -o shpcentrd

shpdata:	shpdata.c $(SHPGOBJ)
	$(CC) $(CFLAGS) shpdata.c ${SHPOBJ} $(LINKOPT) $(GEOOBJ) -o shpdata

shpinfo:	shpinfo.c $(SHPOBJ)
	$(CC) $(CFLAGS) shpinfo.c ${SHPOBJ} $(LINKOPT) $(GEOOBJ) -o shpinfo
	
shpwkb:		shpwkb.c $(SHPGOBJ)
	$(CC) $(CFLAGS) shpwkb.c ${SHPOBJ} $(LINKOPT) $(GEOOBJ) -o shpwkb

shpproj: 	shpproj.c $(SHPOBJ)
	$(CC) $(CFLAGS) shpproj.c  $(SHPOBJ) $(GEOOBJ) -lm -lproj $(LINKOPT) -o shpproj

dbfinfo: 	dbfinfo.c $(SHPOBJ)
	$(CC) $(CFLAGS) dbfinfo.c $(SHPOBJ) $(LINKOPT) -o dbfinfo
	
dbfcat:		dbfcat.c $(SHPOBJ)
	$(CC) $(CFLAGS) dbfcat.c $(SHPOBJ) $(LINKOPT) -o dbfcat
		
check:	testproj
		
testproj: 
	tests/shpproj.sh

	  
