/*
 * Copyright (c) 1995 Frank Warmerdam
 *
 * This code is in the public domain.
 *
 * $Log$
 * Revision 1.3  1995-08-23 02:25:25  warmerda
 * Added support for bounds.
 *
 * Revision 1.2  1995/08/04  03:18:11  warmerda
 * Added header.
 *
 */

static char rcsid[] = 
  "$Id$";

#include "shapefil.h"

int main( int argc, char ** argv )

{
    SHPHandle	hSHP;
    int		nShapeType, nEntities, nVertices, nParts, *panParts, i, iPart;
    double	*padVertices, adBounds[4];
    const char 	*pszPlus;

/* -------------------------------------------------------------------- */
/*      Display a usage message.                                        */
/* -------------------------------------------------------------------- */
    if( argc != 2 )
    {
	printf( "shpdump shp_file\n" );
	exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Open the passed shapefile.                                      */
/* -------------------------------------------------------------------- */
    hSHP = SHPOpen( argv[1], "r" );

    if( hSHP == NULL )
    {
	printf( "Unable to open:%s\n", argv[1] );
	exit( 1 );
    }

    SHPGetInfo( hSHP, &nEntities, &nShapeType );

/* -------------------------------------------------------------------- */
/*      Print out the file bounds.                                      */
/* -------------------------------------------------------------------- */
    SHPReadBounds( hSHP, -1, adBounds );
    printf( "File Bounds: (%lg,%lg) - (%lg,%lg)\n",
	    adBounds[0], adBounds[1], adBounds[2], adBounds[3] );

/* -------------------------------------------------------------------- */
/*	Skim over the list of shapes, printing all the vertices.	*/
/* -------------------------------------------------------------------- */
    for( i = 0; i < nEntities; i++ )
    {
	int		j;

	padVertices = SHPReadVertices( hSHP, i, &nVertices, &nParts,
				       &panParts );
	SHPReadBounds( hSHP, i, adBounds );
	printf( "\nShape:%d  Bounds:(%lg,%lg) - (%lg,%lg)\n",
	        i, adBounds[0], adBounds[1], adBounds[2], adBounds[3] );

	for( j = 0, iPart = 1; j < nVertices; j++ )
	{
	    if( iPart < nParts && panParts[iPart] == j+1 )
	    {
		iPart++;
		pszPlus = "+";
	    }
	    else
	        pszPlus = "";

	    printf("(%lg,%lg) %s \n", 
		   padVertices[j*2], padVertices[j*2+1], 
		   pszPlus );
	}
    }

    SHPClose( hSHP );
}
