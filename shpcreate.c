/*
 * Copyright (c) 1995 Frank Warmerdam
 *
 * This code is in the public domain.
 *
 * $Log$
 * Revision 1.2  1995-08-04 03:16:43  warmerda
 * Added header.
 *
 */

static char rcsid[] = 
  "$Id$";

#include "shapefil.h"

int main( int argc, char ** argv )

{
    SHPHandle	hSHP;
    int		nShapeType;
    double	*padVertices;

/* -------------------------------------------------------------------- */
/*      Display a usage message.                                        */
/* -------------------------------------------------------------------- */
    if( argc != 3 )
    {
	printf( "shpcreate shp_file [point/arc/polygon/multipoint]\n" );
	exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*	Figure out the shape type.					*/
/* -------------------------------------------------------------------- */
    if( strcmp(argv[2],"POINT") == 0 || strcmp(argv[2],"point") == 0 )
        nShapeType = SHPT_POINT;
    else if( strcmp(argv[2],"ARC") == 0 || strcmp(argv[2],"arc") == 0 )
        nShapeType = SHPT_ARC;
    else if( strcmp(argv[2],"POLYGON") == 0 || strcmp(argv[2],"polygon") == 0 )
        nShapeType = SHPT_POLYGON;
    else if( strcmp(argv[2],"MULTIPOINT")==0 ||strcmp(argv[2],"multipoint")==0)
        nShapeType = SHPT_MULTIPOINT;
    else
    {
	printf( "Shape Type `%s' not recognised.\n", argv[2] );
	exit( 2 );
    }

/* -------------------------------------------------------------------- */
/*	Create the requested layer.					*/
/* -------------------------------------------------------------------- */
    hSHP = SHPCreate( argv[1], nShapeType );

    if( hSHP == NULL )
    {
	printf( "Unable to create:%s\n", argv[1] );
	exit( 3 );
    }

    SHPClose( hSHP );
}
