#include "shapefil.h"

int main( int argc, char ** argv )

{
    SHPHandle	hSHP;
    int		nShapeType, nEntities, nVertices, nParts, *panParts, i, iPart;
    double	*padVertices;
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
/*	Skim over the list of shapes, printing all the vertices.	*/
/* -------------------------------------------------------------------- */
    for( i = 0; i < nEntities; i++ )
    {
	int		j;

	padVertices = SHPReadVertices( hSHP, i, &nVertices, &nParts,
				       &panParts );

	printf( "\nShape:%d\n", i );
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
