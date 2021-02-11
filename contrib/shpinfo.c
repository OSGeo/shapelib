/******************************************************************************
 * Copyright (c) 1999, Carl Anderson
 *
 * This code is based in part on the earlier work of Frank Warmerdam
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 */

#include <stdlib.h>
#include <string.h>
#include "shapefil.h"

int main( int argc, char ** argv )

{
    SHPHandle	hSHP;
    int		nShapeType, nEntities;
    double	adfBndsMin[4], adfBndsMax[4];
    char	sType [15]= "";
/* -------------------------------------------------------------------- */
/*      Display a usage message.                                        */
/* -------------------------------------------------------------------- */
    if( argc != 2 )
    {
	printf( "shpinfo shp_file\n" );
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

    SHPGetInfo( hSHP, &nEntities, &nShapeType, adfBndsMin, adfBndsMax );
    
    switch ( nShapeType ) {
       case SHPT_POINT:
		strcpy(sType,"Point");
		break;

       case SHPT_ARC:
		strcpy(sType,"Polyline");
		break;

       case SHPT_POLYGON:
		strcpy(sType,"Polygon");
		break;

       case SHPT_MULTIPOINT:
		strcpy(sType,"MultiPoint");
		break;
        }

/* -------------------------------------------------------------------- */
   printf ("Info for %s\n",argv[1]);
   printf ("%s(%d), %d Records in file\n",sType,nShapeType,nEntities);

/* -------------------------------------------------------------------- */
/*      Print out the file bounds.                                      */
/* -------------------------------------------------------------------- */
    printf( "File Bounds: (%15.10lg,%15.10lg)\n\t(%15.10lg,%15.10lg)\n",
	    adfBndsMin[0], adfBndsMin[1], adfBndsMax[0], adfBndsMax[1] );



    SHPClose( hSHP );
}
