/*
 * ORGINAL CODE WAS FROM "dbfdump.c", "shpdump.c", and "shpopen.c".
 *    Frank Warmerdam 1995
 *
 * This code is in the public domain.
 *
 * $Log$
 * Revision 1.2  1998-06-18 01:19:49  warmerda
 * Made C++ compilable.
 *
 * Revision 1.1  1997/05/27 20:40:27  warmerda
 * Initial revision
 *
 *
 * Altered "shpdump" and "dbfdump" to allow two files to be appended.
 * Other Functions:
 *    Selecting from the DBF before the write occurs.
 *    Change the UNITS between Feet and Meters and Shift X,Y.
 *    Clip and Erase boundary.
 *
 *    Bill Miller   NC-DOT -- Feb. 1997 -- bmiller@doh.dot.state.nc.us
 *         There was not a lot of time to debug hidden problems;
 *         And the code is not very well organized or documented.
 *         The clip/erase function was not well tested.
 *
 *    PURPOSE: I needed a program to Append, Select, Change Unit, and 
 *             Clip boundaries.  The program only passes thru the
 *             data once.
 *
 */

static char rcsid[] = 
  "$Id$";

#include "shapefil.h"
#include "string.h"
#ifndef FALSE
#  define FALSE		0
#  define TRUE		1
#endif

char            infile[80], outfile[80], temp[400];

/* Variables for shape files */
SHPHandle	hSHP;
SHPHandle	hSHPappend;
int		nShapeType, nEntities, nVertices, nParts, *panParts, iPart;
int		nShapeTypeAppend, nEntitiesAppend;
double		*padVertices, adBounds[4];


/* Variables for DBF files */
DBFHandle	hDBF;
DBFHandle	hDBFappend;
    
DBFFieldType    iType;
DBFFieldType    jType;
    
char	iszTitle[12];
char	jszTitle[12];

int	*pt;
char	iszFormat[32], iszField[1024];
char	jszFormat[32], jszField[1024];
int	i, ti, iWidth, iDecimals, iRecord;
int	j, tj, jWidth, jDecimals, jRecord;
int     found, newdbf;

void openfiles(void);
void setext(char *pt, char *ext);
int strncasecmp2(char *s1, char *s2, int n);
void mergefields(void);
void findselect(void);
void showitems(void);
int selectrec();
int check_theme_bnd();
int clip();
void error();


/* -------------------------------------------------------------------- */
/* Variables for the SELECT function */
/* -------------------------------------------------------------------- */
   char    selectitem[40], *cpt;
   int     selectvalues[150];
   int     iselect = FALSE, iselectitem = -1, selcount=0;
   int     iunselect = FALSE;

/* -------------------------------------------------------------------- */
/* Variables for the CLIP and ERASE functions */
/* -------------------------------------------------------------------- */
   double  cxmin, cymin, cxmax, cymax; 
   int     iclip = FALSE, ierase = FALSE;
   int     itouch = FALSE, iinside = FALSE, icut = FALSE;
   int     ibound = FALSE, ipoly = FALSE;
   char    clipfile[80];

/* -------------------------------------------------------------------- */
/* Variables for the UNIT function */
/* -------------------------------------------------------------------- */
   double  factor = 1;  /* NO FACTOR */
   int     iunit = FALSE;
   
   
/* -------------------------------------------------------------------- */
/* Variables for the SHIFT function */
/* -------------------------------------------------------------------- */
   double  xshift = 0, yshift = 0;  /* NO SHIFT */
      
int main( int argc, char ** argv )
{

/* -------------------------------------------------------------------- */
/*      Check command line usage.                                       */
/* -------------------------------------------------------------------- */
    if( argc < 2 ) error();
    strcpy(infile, argv[1]);
    if (argc == 2 ) {
        setext(infile, "shp");
        printf("DESCRIBE: %s\n",infile);
        strcpy(outfile,"");
    } else {
        strcpy(outfile,argv[2]);
    }
/* -------------------------------------------------------------------- */
/*	Look for other functions on the command line. (SELECT, UNIT)  	*/
/* -------------------------------------------------------------------- */
    for (i = 3; i < argc; i++)
    {
    	if ((strncasecmp2(argv[i],  "SEL",3) == 0) ||
           (strncasecmp2(argv[i],  "UNSEL",5) == 0))
    	{
            if (strncasecmp2(argv[i],  "UNSEL",5) == 0) iunselect=TRUE;
    	    i++;
    	    if (i >= argc) error();
    	    strcpy(selectitem,argv[i]);
    	    i++;
    	    if (i >= argc) error();
    	    selcount=0;
    	    strcpy(temp,argv[i]);
    	    cpt=temp;
    	    tj = atoi(cpt);
    	    ti = 0;
    	    while (tj>0) {
    	       selectvalues[selcount] = tj;
    	       while( *cpt >= '0' && *cpt <= '9')
    	          cpt++; 
    	       while( *cpt > '\0' && (*cpt < '0' || *cpt > '9') )
    	          cpt++; 
    	       tj=atoi(cpt);
    	       selcount++;
    	    }
    	    iselect=TRUE;
    	}
    	else
        if ((strncasecmp2(argv[i], "CLIP",4) == 0) ||
           (strncasecmp2(argv[i],  "ERASE",5) == 0))
        {
            if (strncasecmp2(argv[i],  "ERASE",5) == 0) ierase=TRUE;
            i++;
    	    if (i >= argc) error();
    	    strcpy(clipfile,argv[i]);
            sscanf(argv[i],"%lf",&cxmin);
            i++;
    	    if (i >= argc) error();
            if (strncasecmp2(argv[i],  "BOUND",5) == 0) {
                 setext(clipfile, "shp");
                 hSHP = SHPOpen( clipfile, "rb" );
                 if( hSHP == NULL )
                 {
                   printf( "ERROR: Unable to open the clip shape file:%s\n", clipfile );
                   exit( 1 );
                 }
                 SHPReadBounds( hSHP, -1, adBounds );
                 cxmin = adBounds[0];
                 cymin = adBounds[1];
                 cxmax = adBounds[2];
                 cymax = adBounds[3];
                 printf("Theme Clip Boundary: (%lf,%lf) - (%lf,%lf)\n",cxmin, cymin, cxmax, cymax);
                 ibound=TRUE;
                 }
            else if (strncasecmp2(argv[i],  "POLY",4) == 0)
                 {
                 ipoly=TRUE;
                 }
            else {  /*** xmin,ymin,xmax,ymax ***/
                 sscanf(argv[i],"%lf",&cymin);
                 i++;
    	         if (i >= argc) error();
                 sscanf(argv[i],"%lf",&cxmax);
                 i++;
    	         if (i >= argc) error();
                 sscanf(argv[i],"%lf",&cymax);
                 printf("Clip Box: (%lf,%lf) - (%lf,%lf)\n",cxmin, cymin, cxmax, cymax);
                 }
            i++;
    	    if (i >= argc) error();
            if (strncasecmp2(argv[i],  "CUT",3) == 0) icut=TRUE;
            else if (strncasecmp2(argv[i],  "TOUCH",5) == 0) itouch=TRUE;
                 else if (strncasecmp2(argv[i],  "INSIDE",6) == 0) iinside=TRUE;
                      else error();
            iclip=TRUE;
        }
    	else
        if (strncasecmp2(argv[i],  "UNIT",4) == 0)
        {
            i++;
    	    if (i >= argc) error();
            if (strncasecmp2(argv[i],  "METER",5) == 0)
                factor=0.304800609601;
            else
            {
                if (strncasecmp2(argv[i],  "FEET",4) == 0)
                    factor=3.280833;
                else
                    sscanf(argv[i],"%lf",&factor);
            }      
            if (factor == 0) error();
            iunit=TRUE;
            printf("Output file coordinate values will be factored by %lg\n",factor);
    	}
    	else
        if (strncasecmp2(argv[i],"SHIFT",5) == 0)
        {
            i++;
    	    if (i >= argc) error();
            sscanf(argv[i],"%lf",&xshift);
            i++;
    	    if (i >= argc) error();
            sscanf(argv[i],"%lf",&yshift);
            iunit=TRUE;
            printf("X Shift: %lg   Y Shift: %lg\n",xshift,yshift);
        }
        else
        {
    	printf("ERROR: Unknown function %s\n",argv[i]);  error();
    	}
    }
/* -------------------------------------------------------------------- */
/*	If there is no data in this file let the user know.		*/
/* -------------------------------------------------------------------- */
    openfiles();  /* Open the infile and the outfile for shape and dbf. */
    if( DBFGetFieldCount(hDBF) == 0 )
    {
	puts( "There are no fields in this table!" );
	exit( 1 );
    }
/* -------------------------------------------------------------------- */
/*      Print out the file bounds.                                      */
/* -------------------------------------------------------------------- */
    iRecord = DBFGetRecordCount( hDBF );
    SHPReadBounds( hSHP, -1, adBounds );
    printf( "Input Bounds:  (%lg,%lg) - (%lg,%lg)   Entities: %d   DBF: %d\n",
	    adBounds[0], adBounds[1], adBounds[2], adBounds[3], nEntities, iRecord );
	    
    if (strcmp(outfile,"") == 0)
    	{
    	ti = DBFGetFieldCount( hDBF );
	showitems();
	exit(0);
	}
     
    if (iclip) check_theme_bnd();
    
    jRecord = DBFGetRecordCount( hDBFappend );
    SHPReadBounds( hSHPappend, -1, adBounds );
    if (nEntitiesAppend == 0)
         puts("New Output File");
    else
         printf( "Append Bounds: (%lg,%lg) - (%lg,%lg)   Entities: %d  DBF: %d\n",
	    adBounds[0], adBounds[1], adBounds[2], adBounds[3], nEntitiesAppend, jRecord );
    
/* -------------------------------------------------------------------- */
/*	Find matching fields in the append file or add new items.       */
/* -------------------------------------------------------------------- */
    mergefields();
/* -------------------------------------------------------------------- */
/*	Find selection field if needed.                                 */
/* -------------------------------------------------------------------- */
    if (iselect)    findselect();
    

/* -------------------------------------------------------------------- */
/*  Read all the records 						*/
/* -------------------------------------------------------------------- */
    jRecord = DBFGetRecordCount( hDBFappend );
    for( iRecord = 0; iRecord < nEntities; iRecord++)  /** DBFGetRecordCount(hDBF) **/
    {
    
/* -------------------------------------------------------------------- */
/*      SELECT for values if needed. (Can the record be skipped.)       */
/* -------------------------------------------------------------------- */
        if (iselect)
            if (selectrec() == 0) goto SKIP_RECORD;   /** SKIP RECORD **/

/* -------------------------------------------------------------------- */
/*      Read a Shape record                                             */
/* -------------------------------------------------------------------- */
	padVertices = SHPReadVertices( hSHP, iRecord, &nVertices, &nParts, &panParts );

/* -------------------------------------------------------------------- */
/*      Clip coordinates of shapes if needed.                           */
/* -------------------------------------------------------------------- */
        if (iclip)
            if (clip() == 0) goto SKIP_RECORD; /** SKIP RECORD **/

/* -------------------------------------------------------------------- */
/*      Read a DBF record and copy each field.                          */
/* -------------------------------------------------------------------- */
	for( i = 0; i < DBFGetFieldCount(hDBF); i++ )
	{
/* -------------------------------------------------------------------- */
/*      Store the record according to the type and formatting           */
/*      information implicit in the DBF field description.              */
/* -------------------------------------------------------------------- */
          if (pt[i] > -1)  /* if the current field exists in output file */
          {
	    switch( DBFGetFieldInfo( hDBF, i, NULL, &iWidth, &iDecimals ) )
	    {
	      case FTString:
	        DBFWriteStringAttribute(hDBFappend, jRecord, pt[i],
	        (DBFReadStringAttribute( hDBF, iRecord, i )) );
		break;

	      case FTInteger:
	        DBFWriteIntegerAttribute(hDBFappend, jRecord, pt[i],
	        (DBFReadIntegerAttribute( hDBF, iRecord, i )) );
		break;

	      case FTDouble:
	        DBFWriteDoubleAttribute(hDBFappend, jRecord, pt[i],
	        (DBFReadDoubleAttribute( hDBF, iRecord, i )) );
		break;
	    }
	  }
	}
	jRecord++;
/* -------------------------------------------------------------------- */
/*      Change UNIT and SHIFT coordinates of shapes if needed.          */
/* -------------------------------------------------------------------- */
        if (iunit)
        {
	    for( j = 0; j < (nVertices*2); j++ ) 
	    {
	        padVertices[j] = padVertices[j] * factor + xshift; j++;
	        padVertices[j] = padVertices[j] * factor + yshift;
	    }
        }
        
/* -------------------------------------------------------------------- */
/*      Write the Shape record                                          */
/* -------------------------------------------------------------------- */
        SHPWriteVertices(hSHPappend, nVertices, nParts, panParts, padVertices );
        SKIP_RECORD:
        j=0;
    }

/* -------------------------------------------------------------------- */
/*      Print out the # of Entities and the file bounds.                */
/* -------------------------------------------------------------------- */
    jRecord = DBFGetRecordCount( hDBFappend );
    SHPGetInfo( hSHPappend, &nEntitiesAppend, &nShapeTypeAppend );
    SHPReadBounds( hSHPappend, -1, adBounds );
    printf( "Output Bounds: (%lg,%lg) - (%lg,%lg)   Entities: %d  DBF: %d\n\n",
	    adBounds[0], adBounds[1], adBounds[2], adBounds[3], nEntitiesAppend, jRecord );

/* -------------------------------------------------------------------- */
/*      Close the both shapefiles.                                      */
/* -------------------------------------------------------------------- */
    SHPClose( hSHP );
    SHPClose( hSHPappend );
    DBFClose( hDBF );
    DBFClose( hDBFappend );
    return( 0 );
}


/************************************************************************/
/*                             openfiles()                              */
/************************************************************************/

void openfiles() {
/* -------------------------------------------------------------------- */
/*      Open the DBF file.                                              */
/* -------------------------------------------------------------------- */
    setext(infile, "dbf");
    hDBF = DBFOpen( infile, "rb" );
    if( hDBF == NULL )
    {
	printf( "ERROR: Unable to open the input DBF:%s\n", infile );
	exit( 1 );
    }
/* -------------------------------------------------------------------- */
/*      Open the append DBF file.                                       */
/* -------------------------------------------------------------------- */
    if (strcmp(outfile,"")) {
    setext(outfile, "dbf");
    hDBFappend = DBFOpen( outfile, "rb+" );
    newdbf=0;
    if( hDBFappend == NULL )
    {
        newdbf=1;
        hDBFappend = DBFCreate( outfile );
        if( hDBFappend == NULL )
        {
            printf( "ERROR: Unable to open the append DBF:%s\n", outfile );
	    exit( 1 );
        }
    }
    }
/* -------------------------------------------------------------------- */
/*      Open the passed shapefile.                                      */
/* -------------------------------------------------------------------- */
    setext(infile, "shp");
    hSHP = SHPOpen( infile, "rb" );

    if( hSHP == NULL )
    {
	printf( "ERROR: Unable to open the input shape file:%s\n", infile );
	exit( 1 );
    }

    SHPGetInfo( hSHP, &nEntities, &nShapeType );

/* -------------------------------------------------------------------- */
/*      Open the passed append shapefile.                               */
/* -------------------------------------------------------------------- */
    if (strcmp(outfile,"")) {
    setext(outfile, "shp");
    hSHPappend = SHPOpen( outfile, "rb+" );

    if( hSHPappend == NULL )
    {
        hSHPappend = SHPCreate( outfile, nShapeType );
        if( hSHPappend == NULL )
        {
            printf( "ERROR: Unable to open the append shape file:%s\n", outfile );
	    exit( 1 );
        }
    }
    SHPGetInfo( hSHPappend, &nEntitiesAppend, &nShapeTypeAppend );

    if (nShapeType != nShapeTypeAppend) 
    {
	puts( "ERROR: Input and Append shape files are of different types.");
	exit( 1 );
    }
    }
  
}

/* -------------------------------------------------------------------- */
/*	Change the extension.  If there is any extension on the 	*/
/*	filename, strip it off and add the new extension                */
/* -------------------------------------------------------------------- */
void setext(char *pt, char *ext)
{
int i;
    for( i = strlen(pt)-1; 
	 i > 0 && pt[i] != '.' && pt[i] != '/' && pt[i] != '\\';
	 i-- ) {}

    if( pt[i] == '.' )
        pt[i] = '\0';
        
    strcat(pt,".");
    strcat(pt,ext);
}



/* -------------------------------------------------------------------- */
/*	Find matching fields in the append file.                        */
/*      Output file must have zero records to add any new fields.       */
/* -------------------------------------------------------------------- */
void mergefields()
{
    int i,j;
    ti = DBFGetFieldCount( hDBF );
    tj = DBFGetFieldCount( hDBFappend );
    /* Create a pointer array for the max # of fields in the output file */
    pt = (int *) malloc( (ti+tj+1) * sizeof(int) ); 
    
    for( i = 0; i < ti; i++ )
    {
       pt[i]= -1;  /* Initial pt values to -1 */
    }
    /* DBF must be empty before adding items */
    jRecord = DBFGetRecordCount( hDBFappend );
    for( i = 0; i < ti; i++ )
    {
	iType = DBFGetFieldInfo( hDBF, i, iszTitle, &iWidth, &iDecimals );
        found=0;
        {
      	    for( j = 0; j < tj; j++ )   /* Search all field names for a match */
    	    {
	        jType = DBFGetFieldInfo( hDBFappend, j, jszTitle, &jWidth, &jDecimals );
	        if (iType == jType && (strcmp(iszTitle, jszTitle) == 0) )
	        {
	            if (found == 1  ||  newdbf == 1)
	            {
	                if (i == j)  pt[i]=j;
	                printf("Warning: Duplicate field name found (%s)\n",iszTitle);
	                /* Duplicate field name
	                   (Try to guess the correct field by position) */
	            }
	            else
	            {
	            	pt[i]=j;  found=1; 
	            }
	        }
	    }
	}
	
	if (pt[i] == -1  && found == 0)  /* Try to force into an existing field */
	{                                /* Ignore the field name, width, and decimal places */
	    jType = DBFGetFieldInfo( hDBFappend, j, jszTitle, &jWidth, &jDecimals );
	    if (iType == jType) 
	    {
	    	pt[i]=i;  found=1;
	    }
	}
	if (found == 0  &&  jRecord == 0)  /* Add missing field to the append table */
	{                 /* The output DBF must be is empty */
	    pt[i]=tj;
	    tj++;
	    if( !DBFAddField( hDBFappend, iszTitle, iType, iWidth, iDecimals ))
	    {
		printf( "Warning: DBFAddField(%s, TYPE:%d, WIDTH:%d  DEC:%d, ITEM#:%d of %d) failed.\n",
		         iszTitle, iType, iWidth, iDecimals, (i+1), (ti+1) );
		pt[i]=-1;
	    }
	}
    }
}


void findselect()
{
    /* Find the select field name */
    iselectitem = -1;
    for( i = 0; i < ti  &&  iselectitem < 0; i++ )
    {
	iType = DBFGetFieldInfo( hDBF, i, iszTitle, &iWidth, &iDecimals );
        if (strncasecmp2(iszTitle, selectitem, 0) == 0) iselectitem = i;
    }
    if (iselectitem == -1) 
    {
        printf("Warning: Item not found for selection (%s)\n",selectitem);
        iselect = 0;
	showitems();
        printf("Continued... (Selecting entire file)\n");
    }
    /* Extract all of the select values (by field type) */
    
}

void showitems()
{
        printf("Available Items: ");
        for( i = 0; i < ti; i++ )
        {
	    iType = DBFGetFieldInfo( hDBF, i, iszTitle, &iWidth, &iDecimals );
	    printf("%s, ",iszTitle);
        }
        printf("(total=%d)\n",ti);
}

int selectrec()
{
int value, ty;

   ty = DBFGetFieldInfo( hDBF, iselectitem, NULL, &iWidth, &iDecimals);
      switch(ty)
      {
      case FTString:
	break;
      case FTInteger:
        value = DBFReadIntegerAttribute( hDBF, iRecord, iselectitem );
        for (j = 0; j<selcount; j++)
          {
          if (selectvalues[j] == value)
               if (iunselect) return(0);  /* Keep this record */
                        else  return(1);  /* Skip this record */
          }
	break;
      case FTDouble:
        break;
      }
      if (iunselect) return(1);  /* Skip this record */
               else  return(0);  /* Keep this record */
}


int check_theme_bnd()
{
    if ( (adBounds[0] >= cxmin) && (adBounds[2] <= cxmax) &&
         (adBounds[1] >= cymin) && (adBounds[3] <= cymax) )
    {   /** Theme is totally inside clip area **/
        if (ierase) nEntities=0; /** SKIP THEME  **/
        else   iclip=FALSE; /** WRITE THEME (Clip not needed) **/
    }
            
    if ( ( (adBounds[0] < cxmin) && (adBounds[2] < cxmin) ) ||
         ( (adBounds[1] < cymin) && (adBounds[3] < cymin) ) ||
         ( (adBounds[0] > cxmax) && (adBounds[2] > cxmax) ) ||
         ( (adBounds[1] > cymax) && (adBounds[3] > cymax) ) )
    {   /** Theme is totally outside clip area **/
        if (ierase) iclip=FALSE; /** WRITE THEME (Clip not needed) **/
        else   nEntities=0; /** SKIP THEME  **/
    }
            
    if (nEntities == 0)
        puts("WARNING: Theme is outside the clip area."); /** SKIP THEME  **/
}

int clip()
{
    int  outside=FALSE;
    int  j2=0, i2=0;

    /*** FIRST check the boundary of the feature ***/
    SHPReadBounds( hSHP, iRecord, adBounds );

    if ( (adBounds[0] >= cxmin) && (adBounds[2] <= cxmax) &&
         (adBounds[1] >= cymin) && (adBounds[3] <= cymax) )
    {   /** Feature is totally inside clip area **/
        if (ierase) return(0); /** SKIP  RECORD **/
        else   return(1); /** WRITE RECORD **/
    }
            
    if ( ( (adBounds[0] < cxmin) && (adBounds[2] < cxmin) ) ||
         ( (adBounds[1] < cymin) && (adBounds[3] < cymin) ) ||
         ( (adBounds[0] > cxmax) && (adBounds[2] > cxmax) ) ||
         ( (adBounds[1] > cymax) && (adBounds[3] > cymax) ) )
    {   /** Feature is totally outside clip area **/
        if (ierase) return(1); /** WRITE RECORD **/
        else   return(0); /** SKIP  RECORD **/
    }
       
    if (itouch)
    {
        if (ierase) return(0); /** SKIP  RECORD **/
        else   return(1); /** WRITE RECORD  **/
    }
            
    if (iinside)
    {
        if (ierase) return(1); /** WRITE RECORD **/
        else   return(0); /** SKIP  RECORD **/
    }
           
    /*** SECOND check each vertex in the feature ***/
    for( j2 = 0; j2 < (nVertices*2); j2=j2+2 ) 
    {
        if (padVertices[j2] < cxmin  ||  padVertices[j2] > cxmax)
        {
            outside=TRUE;
        }
        else
        {
            if (padVertices[j2+1] < cymin  ||  padVertices[j2+1] > cymax)
                outside=TRUE;
            else
                outside=FALSE;
        }
        
        
        if (icut)
        {
            if (outside)
            {
            } else {
                if (i2 != j2)
                {
                    padVertices[i2]=padVertices[j2];     /** write vertex **/
                    padVertices[i2+1]=padVertices[j2+1]; /** write vertex **/
                }
                i2=i2+2;
            }
        }
        else
            if (outside)  /* vertex is outside boundary */
            {
                if (iinside)
                {
                    if (ierase) return(1); /** WRITE RECORD **/
                    else   return(0); /** SKIP RECORD **/
                }
            }
            else         /* vertex is inside boundary */
            {
                if (itouch) 
                {
                    if (ierase) return(0); /** SKIP RECORD  **/
                    else   return(1); /** WRITE RECORD **/
                }
            }
    }
    
    if (icut)
    {
        j2=nVertices;
        if (i2 < 4) return(0); /** SKIP RECORD **/
        nVertices=i2/2;
        printf("Vertices:%d   OUT:%d   Number of Parts:%d  PanParts:%d   PadVertices:%d\n",
               j2,nVertices,nParts,panParts,padVertices);
    }
    if (itouch)
    {
        if (ierase) return(1); /** WRITE RECORD **/
        else   return(0); /** SKIP RECORD  **/
    }
    if (iinside)   
    {
        if (ierase) return(0); /** SKIP RECORD  **/
        else   return(1); /** WRITE RECORD **/
    }
}

/* -------------------------------------------------------------------- */
/*      Display a usage message.                                        */
/* -------------------------------------------------------------------- */
void error()
    {
	puts( "USAGE: shputils  <DescribeShape>");
	puts( "USAGE: shputils  <InputShape>  <AppendShape>" );
	puts( "      { <SELECT>   <Item> <valuelist> }" );
	puts( "      { <UNSELECT> <Item> <valuelist> }" );
	puts( "      { <CLIP>   <xmin> <ymin> <xmax> <ymax> <TOUCH|INSIDE|CUT> }" );
	puts( "      { <CLIP>   <Theme>  <BOUNDARY|POLYGON> <TOUCH|INSIDE|CUT> }" );
	puts( "      Clip functions for Cut and Polygon are not supported yet..." );
	puts( "      { <ERASE>  <xmin> <ymin> <xmax> <ymax> <TOUCH|INSIDE|CUT> }" );
	puts( "      { <ERASE>  <Theme>  <BOUNDARY|POLYGON> <TOUCH|INSIDE|CUT> }" );
	puts( "      { <UNIT>   <FEET|METERS|factor>  }" );
	puts( "      { <SHIFT>  <xshift> <yshift> }\n" );
	
	
	puts( "The program will append to an existing shape file or it will" );
	puts( "create a new file if needed." );
	puts( "Only the items in the first output file will be preserved." );
	puts( "When an item does not match with the append theme then the item");
	puts( "might be placed to an existing item at the same position and type." );
	puts( "  OTHER FUNCTIONS:" );
	puts( "  - Select a group of shapes from a comma separated selection list.");
	puts( "  - UnSelect a group of shapes from a comma separated selection list.");
	puts( "  - Clip boundary extent or by theme boundary." );
	puts( "      Touch writes all the shapes that touch the boundary.");
	puts( "      Inside writes all the shapes that are completely within the boundary.");
	puts( "      *(N/A) Cut will cookie-cut shapes that are touching the boundary.");
	puts( "      Boundary clips are only the min and max of a theme boundary." );
	puts( "      *(N/A) Polygon clips use the polygons within a theme.");
	puts( "  - Erase boundary extent or by theme boundary." );
	puts( "      Erase is the direct opposite of the Clip function." );
	puts( "  - Change coordinate value units between meters and feet.");
	puts( "      There is no way to determine the input unit of a shape file.");
	puts( "      Skip this function if the shape file is already in the correct unit.");
	puts( "      Clip and Erase will be done before the unit is changed.");
	puts( "      A shift will be done after the unit is changed."); 
	puts( "  - Shift X and Y coordinates.\n" );
	puts( "Finally, There can only be one select or unselect in the command line.");
	puts( "         There can only be one clip or erase in the command line.");
	puts( "         There can only be one unit and only one shift in the command line.");
	puts( "EX: shputils in.shp out.shp  CLIP 10 10 90 90 Touch  UNIT Feet  SHIFT 40 40");
	puts( "    shputils in.shp out.shp  SELECT countycode 3,5,9,13,17,27");
	exit( 1 );
    }

/************************************************************************/
/*                            strncasecmp2()                            */
/*                                                                      */
/*      Compare two strings up to n characters                          */
/*      If n=0 then s1 and s2 must be an exact match                    */
/************************************************************************/

int strncasecmp2(char *s1, char *s2, int n)

{
int j,i;
   if (n<1) n=strlen(s1)+1;
   for (i=0; i<n; i++)
   {
      if (*s1 != *s2)
      {
         if (*s1 >= 'a' && *s1 <= 'z') {
            j=*s1-32;
            if (j != *s2) return(1);
         }
         else
         {
            j=*s1+32;
            if (j != *s2) return(1);
         }
      }
      s1++;
      s2++;
   }
   return(0);
}
