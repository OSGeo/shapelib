
#LINKOPT	=	/usr/local/lib/libdbmalloc.a
#CFLAGS	=	-g -DUSE_DBMALLOC

CFLAGS	=	-I..

SHPOBJ	=	../shpopen.o ../dbfopen.o 

default:	all

all:	shp2dxf

shp2dxf:	shp2dxf.c $(SHPOBJ)
	$(CC) $(CFLAGS) shp2dxf.c $(SHPOBJ) $(LINKOPT) -o shp2dxf

