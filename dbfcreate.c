#include "shapefil.h"

int main( int argc, char ** argv )

{
    DBFHandle	hDBF;
    int		*panWidth, i, iRecord;
    char	szFormat[32], szField[1024];
    int		nWidth, nDecimals;

/* -------------------------------------------------------------------- */
/*      Display a usage message.                                        */
/* -------------------------------------------------------------------- */
    if( argc < 2 )
    {
	printf( "dbfcreate xbase_file [[-s field_name width],[-n field_name width decimals]]...\n" );

	exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*	Create the database.						*/
/* -------------------------------------------------------------------- */
    hDBF = DBFCreate( argv[1] );
    if( hDBF == NULL )
    {
	printf( "DBFCreate(%s) failed.\n", argv[1] );
	exit( 2 );
    }
    
/* -------------------------------------------------------------------- */
/*	Loop over the field definitions adding new fields.	       	*/
/* -------------------------------------------------------------------- */
    for( i = 2; i < argc; i++ )
    {
	if( strcmp(argv[i],"-s") == 0 && i < argc-2 )
	{
	    if( !DBFAddField( hDBF, argv[i+1], FTString, atoi(argv[i+2]), 0 ))
	    {
		printf( "DBFAddField(%s,FTString,%d,0) failed.\n",
		        argv[i+1], atoi(argv[i+2]) );
		exit( 4 );
	    }
	    i = i + 2;
	}
	else if( strcmp(argv[i],"-n") == 0 && i < argc-3 )
	{
	    if( !DBFAddField( hDBF, argv[i+1], FTDouble, atoi(argv[i+2]), 
			      atoi(argv[i+3]) ) )
	    {
		printf( "DBFAddField(%s,FTDouble,%d,%d) failed.\n",
		        argv[i+1], atoi(argv[i+2]), atoi(argv[i+3]) );
		exit( 4 );
	    }
	    i = i + 3;
	}
	else
	{
	    printf( "Argument incomplete, or unrecognised:%s\n", argv[i] );
	    exit( 3 );
	}
    }

    DBFClose( hDBF );

    return( 0 );
}
