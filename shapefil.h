#ifndef _SHAPEFILE_H_INCLUDED
#define _SHAPEFILE_H_INCLUDED

/*
 * Copyright (c) 1995 Frank Warmerdam
 *
 * This code is in the public domain.
 *
 * $Log$
 * Revision 1.4  1998-11-09 20:19:33  warmerda
 * Added 3D support, and use of SHPObject.
 *
 * Revision 1.3  1995/08/23 02:24:05  warmerda
 * Added support for reading bounds.
 *
 * Revision 1.2  1995/08/04  03:17:39  warmerda
 * Added header.
 *
 */

#include <stdio.h>

#ifdef USE_DBMALLOC
#include <dbmalloc.h>
#endif

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

    double	adBoundsMin[4];
    double	adBoundsMax[4];

    int		bUpdated;
} SHPInfo;

typedef SHPInfo * SHPHandle;

/* -------------------------------------------------------------------- */
/*      Shape types (nSHPType)                                          */
/* -------------------------------------------------------------------- */
#define SHPT_POINT	1
#define SHPT_ARC	3
#define SHPT_POLYGON	5
#define SHPT_MULTIPOINT	8
#define SHPT_POINTZ	11
#define SHPT_ARCZ	13
#define SHPT_POLYGONZ	15
#define SHPT_MULTIPOINTZ 18
#define SHPT_POINTM	21
#define SHPT_ARCM	23
#define SHPT_POLYGONM	25
#define SHPT_MULTIPOINTM 28
#define SHPT_MULTIPATCH 31


/* -------------------------------------------------------------------- */
/*      Part types - everything but SHPT_MULTIPATCH just uses           */
/*      SHPP_RING.                                                      */
/* -------------------------------------------------------------------- */

#define SHPP_TRISTRIP	0
#define SHPP_TRIFAN	1
#define SHPP_OUTERRING	2
#define SHPP_INNERRING	3
#define SHPP_FIRSTRING	4
#define SHPP_RING	5

/* -------------------------------------------------------------------- */
/*      SHPObject - represents on shape (without attributes) read       */
/*      from the .shp file.                                             */
/* -------------------------------------------------------------------- */
typedef struct
{
    int		nSHPType;

    int		nShapeId; /* -1 is unknown/unassigned */

    int		nParts;
    int		*panPartStart;
    int		*panPartType;
    
    int		nVertices;
    double	*padfX;
    double	*padfY;
    double	*padfZ;
    double	*padfM;

    double	dfXMin;
    double	dfYMin;
    double	dfZMin;
    double	dfMMin;

    double	dfXMax;
    double	dfYMax;
    double	dfZMax;
    double	dfMMax;
} SHPObject;

/* -------------------------------------------------------------------- */
/*      SHP API Prototypes                                              */
/* -------------------------------------------------------------------- */
SHPHandle SHPOpen( const char * pszShapeFile, const char * pszAccess );
SHPHandle SHPCreate( const char * pszShapeFile, int nShapeType );
void	SHPGetInfo( SHPHandle hSHP, int * pnEntities, int * pnShapeType );

SHPObject *SHPReadObject( SHPHandle hSHP, int iShape );
int	SHPWriteObject( SHPHandle hSHP, int iShape, SHPObject * psObject );

void	SHPDestroyObject( SHPObject * psObject );
SHPObject *SHPCreateObject( int nSHPType, int nShapeId,
                            int nParts, int * panPartStart, int * panPartType,
                            int nVertices, double * padfX, double * padfY,
                            double * padfZ, double * padfM );
SHPObject *SHPCreateSimpleObject( int nSHPType, int nVertices,
                              double * padfX, double * padfY, double * padfZ );

void	SHPReadBounds( SHPHandle hSHP, int iShape, double * padBounds );
void	SHPClose( SHPHandle hSHP );

const char *SHPTypeName( int nSHPType );
const char *SHPPartTypeName( int nPartType );

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
