/*
 * Copyright (c) 1995 Frank Warmerdam
 *
 * This code is in the public domain.
 *
 * $Log$
 * Revision 1.6  1998-12-03 15:48:48  warmerda
 * Added report of shapefile type, and total number of shapes.
 *
 * Revision 1.5  1998/11/09 20:57:36  warmerda
 * use SHPObject.
 *
 * Revision 1.4  1995/10/21 03:14:49  warmerda
 * Changed to use binary file access.
 *
 * Revision 1.3  1995/08/23  02:25:25  warmerda
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
    int		nShapeType, nEntities, i, iPart;
    const char 	*pszPlus;
    double 	adfMinBound[4], adfMaxBound[4];

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
    hSHP = SHPOpen( argv[1], "rb" );

    if( hSHP == NULL )
    {
	printf( "Unable to open:%s\n", argv[1] );
	exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Print out the file bounds.                                      */
/* -------------------------------------------------------------------- */
    SHPGetInfo( hSHP, &nEntities, &nShapeType, adfMinBound, adfMaxBound );

    printf( "Shapefile Type: %s   # of Shapes: %d\n\n",
            SHPTypeName( nShapeType ), nEntities );
    
    printf( "File Bounds: (%12.3f,%12.3f,%lg,%lg)\n"
            "         to  (%12.3f,%12.3f,%lg,%lg)\n",
            adfMinBound[0], 
            adfMinBound[1], 
            adfMinBound[2], 
            adfMinBound[3], 
            adfMaxBound[0], 
            adfMaxBound[1], 
            adfMaxBound[2], 
            adfMaxBound[3] );
    
/* -------------------------------------------------------------------- */
/*	Skim over the list of shapes, printing all the vertices.	*/
/* -------------------------------------------------------------------- */
    for( i = 0; i < nEntities; i++ )
    {
	int		j;
        SHPObject	*psShape;

	psShape = SHPReadObject( hSHP, i );

	printf( "\nShape:%d (%s)  nVertices=%d, nParts=%d\n"
                "  Bounds:(%12.3f,%12.3f, %lg, %lg)\n"
                "      to (%12.3f,%12.3f, %lg, %lg)\n",
	        i, SHPTypeName(psShape->nSHPType),
                psShape->nVertices, psShape->nParts,
                psShape->dfXMin, psShape->dfYMin,
                psShape->dfZMin, psShape->dfMMin,
                psShape->dfXMax, psShape->dfYMax,
                psShape->dfZMax, psShape->dfMMax );

	for( j = 0, iPart = 1; j < psShape->nVertices; j++ )
	{
            const char	*pszPartType = "";

            if( j == 0 && psShape->nParts > 0 )
                pszPartType = SHPPartTypeName( psShape->panPartType[0] );
            
	    if( iPart < psShape->nParts
                && psShape->panPartStart[iPart] == j )
	    {
                pszPartType = SHPPartTypeName( psShape->panPartType[iPart] );
		iPart++;
		pszPlus = "+";
	    }
	    else
	        pszPlus = " ";

	    printf("   %s (%12.3f,%12.3f, %lg, %lg) %s \n",
                   pszPlus,
                   psShape->padfX[j],
                   psShape->padfY[j],
                   psShape->padfZ[j],
                   psShape->padfM[j],
                   pszPartType );
	}
        
        SHPDestroyObject( psShape );
    }

    SHPClose( hSHP );

#ifdef USE_DBMALLOC
    malloc_dump(2);
#endif

    exit( 0 );
}
