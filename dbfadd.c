/*
 * Copyright (c) 1995 Frank Warmerdam
 *
 * This code is in the public domain.
 *
 * $Log$
 * Revision 1.2  1995-08-04 03:15:59  warmerda
 * Added header.
 *
 */

static char rcsid[] = 
  "$Id$";

#include "shapefil.h"

int main( int argc, char ** argv )

{
    DBFHandle	hDBF;
    int		i, iRecord;

/* -------------------------------------------------------------------- */
/*      Display a usage message.                                        */
/* -------------------------------------------------------------------- */
    if( argc < 3 )
    {
	printf( "dbfadd xbase_file field_values\n" );

	exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*	Create the database.						*/
/* -------------------------------------------------------------------- */
    hDBF = DBFOpen( argv[1], "r+" );
    if( hDBF == NULL )
    {
	printf( "DBFOpen(%s,\"r+\") failed.\n", argv[1] );
	exit( 2 );
    }
    
/* -------------------------------------------------------------------- */
/*	Do we have the correct number of arguments?			*/
/* -------------------------------------------------------------------- */
    if( DBFGetFieldCount( hDBF ) != argc - 2 )
    {
	printf( "Got %d fields, but require %d\n",
	        argc - 2, DBFGetFieldCount( hDBF ) );
	exit( 3 );
    }

    iRecord = DBFGetRecordCount( hDBF );

/* -------------------------------------------------------------------- */
/*	Loop assigning the new field values.				*/
/* -------------------------------------------------------------------- */
    for( i = 0; i < DBFGetFieldCount(hDBF); i++ )
    {
	if( DBFGetFieldInfo( hDBF, i, NULL, NULL, NULL ) == FTString )
	    DBFWriteStringAttribute(hDBF, iRecord, i, argv[i+2] );
	else
	    DBFWriteDoubleAttribute(hDBF, iRecord, i, atof(argv[i+2]) );
    }

/* -------------------------------------------------------------------- */
/*      Close and cleanup.                                              */
/* -------------------------------------------------------------------- */
    DBFClose( hDBF );

    return( 0 );
}
