/*
 * Copyright (c) 1995 Frank Warmerdam
 *
 * This code is in the public domain.
 *
 * $Log$
 * Revision 1.2  1995-08-04 03:16:22  warmerda
 * Added header.
 *
 */

static char rcsid[] = 
  "$Id$";

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
    if( argc != 2 )
    {
	printf( "dbfdump xbase_file\n" );
	exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Open the file.                                                  */
/* -------------------------------------------------------------------- */
    hDBF = DBFOpen( argv[1], "r" );
    if( hDBF == NULL )
    {
	printf( "DBFOpen(%s,\"r\") failed.\n", argv[1] );
	exit( 2 );
    }
    
/* -------------------------------------------------------------------- */
/*	If there is no data in this file let the user know.		*/
/* -------------------------------------------------------------------- */
    if( DBFGetFieldCount(hDBF) == 0 )
    {
	printf( "There are no fields in this table!\n" );
	exit( 3 );
    }

/* -------------------------------------------------------------------- */
/*	Compute offsets to use when printing each of the field 		*/
/*	values. We make each field as wide as the field title+1, or 	*/
/*	the field value + 1. 						*/
/* -------------------------------------------------------------------- */
    panWidth = (int *) malloc( DBFGetFieldCount( hDBF ) * sizeof(int) );

    for( i = 0; i < DBFGetFieldCount(hDBF); i++ )
    {
	char		szTitle[12];
	DBFFieldType	eType;

	eType = DBFGetFieldInfo( hDBF, i, szTitle, &nWidth, &nDecimals );
	if( strlen(szTitle) > nWidth )
	    panWidth[i] = strlen(szTitle);
	else
	    panWidth[i] = nWidth;

	if( eType == FTString )
	    sprintf( szFormat, "%%-%ds ", panWidth[i] );
	else
	    sprintf( szFormat, "%%%ds ", panWidth[i] );
	printf( szFormat, szTitle );
    }
    printf( "\n" );

/* -------------------------------------------------------------------- */
/*	Read all the records 						*/
/* -------------------------------------------------------------------- */
    for( iRecord = 0; iRecord < DBFGetRecordCount(hDBF); iRecord++ )
    {
	for( i = 0; i < DBFGetFieldCount(hDBF); i++ )
	{
/* -------------------------------------------------------------------- */
/*      Print the record according to the type and formatting           */
/*      information implicit in the DBF field description.              */
/* -------------------------------------------------------------------- */
	    switch( DBFGetFieldInfo( hDBF, i, NULL, &nWidth, &nDecimals ) )
	    {
	      case FTString:
		sprintf( szFormat, "%-%%ds ", nWidth );
		printf( szFormat, 
		        DBFReadStringAttribute( hDBF, iRecord, i ) );
		break;

	      case FTInteger:
		sprintf( szFormat, "%%%dd ", nWidth );
		printf( szFormat, 
		        DBFReadIntegerAttribute( hDBF, iRecord, i ) );
		break;

	      case FTDouble:
		sprintf( szFormat, "%%%d.%dlf ", nWidth, nDecimals );
		printf( szFormat, 
		        DBFReadDoubleAttribute( hDBF, iRecord, i ) );
		break;
	    }

/* -------------------------------------------------------------------- */
/*      Write out any extra spaces required to pad out the field        */
/*      width.                                                          */
/* -------------------------------------------------------------------- */
	    if( panWidth[i] > nWidth )
	    {
		sprintf( szFormat, "%%%ds", panWidth[i] - nWidth );
		printf( szFormat, "" );
	    }

	    fflush( stdout );
	}
	printf( "\n" );
    }

    DBFClose( hDBF );

    return( 0 );
}
