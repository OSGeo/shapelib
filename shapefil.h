#ifndef _SHAPEFILE_H_INCLUDED
#define _SHAPEFILE_H_INCLUDED

/*
 * Copyright (c) 1995 Frank Warmerdam
 *
 * This code is in the public domain.
 *
 * $Log$
 * Revision 1.3  1995-08-23 02:24:05  warmerda
 * Added support for reading bounds.
 *
 * Revision 1.2  1995/08/04  03:17:39  warmerda
 * Added header.
 *
 */

#include <stdio.h>

/************************************************************************/
/*                             SHP Support.                             */
/************************************************************************/
typedef	struct
{
    FILE        *fpSHP;
    FILE	*fpSHX;

    int		nShapeType;				/* SHPT_* */
    int		nFileSize;				/* SHP file */

    int         nRecords;
    int		nMaxRecords;
    int		*panRecOffset;
    int		*panRecSize;

    double	adBoundsMin[2];
    double	adBoundsMax[2];

    int		bUpdated;
} SHPInfo;

typedef SHPInfo * SHPHandle;

#define SHPT_POINT	1
#define SHPT_ARC	3
#define SHPT_POLYGON	5
#define SHPT_MULTIPOINT	8

SHPHandle SHPOpen( const char * pszShapeFile, const char * pszAccess );
SHPHandle SHPCreate( const char * pszShapeFile, int nShapeType );
void	SHPGetInfo( SHPHandle hSHP, int * pnEntities, int * pnShapeType );
double *SHPReadVertices( SHPHandle hSHP, int iShape, int *pnVCount,
			 int * pnPartCount, int ** ppanParts );
int	SHPWriteVertices( SHPHandle hSHP, int nVCount, int nPartCount, 
			  int * panParts, double * pdVertices);
void	SHPReadBounds( SHPHandle hSHP, int iShape, double * padBounds );
void	SHPClose( SHPHandle hSHP );

/************************************************************************/
/*                             DBF Support.                             */
/************************************************************************/
typedef	struct
{
    FILE	*fp;

    int         nRecords;

    int		nRecordLength;
    int		nHeaderLength;
    int		nFields;
    int		*panFieldOffset;
    int		*panFieldSize;
    int		*panFieldDecimals;
    char	*pachFieldType;

    char	*pszHeader;

    int		nCurrentRecord;
    int		bCurrentRecordModified;
    char	*pszCurrentRecord;
    
    int		bNoHeader;
    int		bUpdated;
} DBFInfo;
typedef DBFInfo * DBFHandle;

typedef enum {
  FTString,
  FTInteger,
  FTDouble,
  FTInvalid
} DBFFieldType;

DBFHandle DBFOpen( const char * pszDBFFile, const char * pszAccess );
DBFHandle DBFCreate( const char * pszDBFFile );

int	DBFGetFieldCount( DBFHandle psDBF );
int	DBFGetRecordCount( DBFHandle psDBF );
int	DBFAddField( DBFHandle hDBF, const char * pszFieldName,
		     DBFFieldType eType, int nWidth, int nDecimals );

DBFFieldType DBFGetFieldInfo( DBFHandle psDBF, int iField, 
			      char * pszFieldName, 
			      int * pnWidth, int * pnDecimals );

int 	DBFReadIntegerAttribute( DBFHandle hDBF, int iShape, int iField );
double 	DBFReadDoubleAttribute( DBFHandle hDBF, int iShape, int iField );
const char *DBFReadStringAttribute( DBFHandle hDBF, int iShape, int iField );

int DBFWriteIntegerAttribute( DBFHandle hDBF, int iShape, int iField, 
			      int nFieldValue );
int DBFWriteDoubleAttribute( DBFHandle hDBF, int iShape, int iField,
			     double dFieldValue );
int DBFWriteStringAttribute( DBFHandle hDBF, int iShape, int iField,
			     const char * pszFieldValue );

void	DBFClose( DBFHandle hDBF );

#endif /* ndef _SHAPEFILE_H_INCLUDED */
