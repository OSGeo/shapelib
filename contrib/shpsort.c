/******************************************************************************
 * Copyright (c) 2004, Eric G. Miller
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
 * shpsort
 *
 * Rewrite a shapefile sorted by a field or by the geometry.  For polygons,
 * sort by area, for lines sort by length and do nothing for all others.
 *
 * $Log$
 * Revision 1.1  2004-06-24 00:55:34  fwarmerdam
 * New
 *
 * Revision 1.2  2004/06/23 23:19:58  emiller
 * use tuple copy, misc changes
 *
 * Revision 1.1  2004/06/23 21:38:17  emiller
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "shapefil.h"

#undef DEBUG

static int * index_shape (SHPHandle shp, int ascending);
static int * index_dbf (DBFHandle dbf, int field, int ascending);
static char * dupstr (const char *);
static void copy_related (const char *inName, const char *outName, 
			  const char *old_ext, const char *new_ext);

int main (int argc, char *argv[]) {

  SHPHandle inSHP, outSHP;
  DBFHandle inDBF, outDBF;
  int       shpType, count;
  int       fieldIndex;
  int       ascending = 1;
  int       len; 
  int       i;
  char      *fieldName;
  int       *index;
  int       j;
  SHPObject *feat;
  char *tuple;

  if (argc < 4) {
    printf("USAGE: shpsort infile outfile field [ASCENDING | DESCENDING]\n");
    exit(EXIT_FAILURE);
  }

  inSHP = SHPOpen (argv[1], "rb");
  if (!inSHP) {
    fputs("Couldn't open shapefile for reading!\n", stderr);
    exit(EXIT_FAILURE);
  }
  SHPGetInfo(inSHP, &count, &shpType, NULL, NULL);

  /* If we can open the inSHP, open its DBF */
  inDBF = DBFOpen (argv[1], "rb");
  if (!inDBF) {
    fputs("Couldn't open dbf file for reading!\n", stderr);
    exit(EXIT_FAILURE);
  }

  /* Check field existence */
  len = (int)strlen(argv[3]);
  fieldName = malloc(len + 1);
  if (!fieldName) {
    fputs("malloc failed!\n", stderr);
    exit(EXIT_FAILURE);
  }
  for (i = 0; i <= len; i++)
    fieldName[i] = (char)toupper((unsigned char)argv[3][i]);

  if (strcmp(fieldName, "SHAPE") == 0) {
    fieldIndex = -1;
  }
  else if (strcmp(fieldName, "FID") == 0) {
    fieldIndex = -2;
  }
  else {
    fieldIndex = DBFGetFieldIndex(inDBF, fieldName);
    if (fieldIndex < 0) {
      fputs("DBF field not found!\n", stderr);
      exit (EXIT_FAILURE);
    }
  }

  /* check direction */
  if (argc > 4)
    if (strcmp(argv[4], "DESCENDING") == 0)
      ascending = 0;

  /* build sorted index */
  if (fieldIndex == -1) {
    /* sorting by shape */
    index = index_shape(inSHP, ascending);
  }
  else if (fieldIndex == -2) {
    /* copy via "FID" virtual field in forward or reverse */
    index = malloc (sizeof *index * count);
    if (!index) {
      fprintf(stderr, "%s:%d: malloc failed!\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
    }
    if (ascending)
      for (i = 0; i < count; i++)
	index[i] = i;
    else
      for (i = 0, j = count - 1; i < count; i++, j--)
	index[i] = j;
#ifdef DEBUG
    printf("DEBUG: FID ordering ascending --> %d {\n", ascending);
    for (i =0; i < count; i++)
      printf("%d\n", index[i]);
    puts("}\n");
#endif
  }
  else {
    index = index_dbf(inDBF, fieldIndex, ascending);
  }

  /* Create output shapefile */
  outSHP = SHPCreate(argv[2], shpType);
  if (!outSHP) {
    fprintf(stderr, "%s:%d: couldn't create output shapefile!\n",
	    __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }
  
  /* Create output dbf */
  outDBF = DBFCloneEmpty(inDBF, argv[2]);
  if (!outDBF) {
    fprintf(stderr, "%s:%d: couldn't create output dBASE file!\n",
	    __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }

  /* Copy projection file, if any */
  copy_related(argv[1], argv[2], ".shp", ".prj");

  /* Copy metadata file, if any */
  copy_related(argv[1], argv[2], ".shp", ".shp.xml");

  /* Write out sorted results */
  for (i = 0; i < count; i++) {
    feat = SHPReadObject(inSHP, index[i]);
    if (SHPWriteObject(outSHP, -1, feat) < 0) {
      fprintf(stderr, "%s:%d: error writing shapefile!\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
    }
    tuple = (char *) DBFReadTuple(inDBF, index[i]);
    if (DBFWriteTuple(outDBF, i, tuple) < 0) {
      fprintf(stderr, "%s:%d: error writing dBASE file!\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
    }
  }
  SHPClose(inSHP);
  SHPClose(outSHP);
  DBFClose(inDBF);
  DBFClose(outDBF);

  return EXIT_SUCCESS;

}

static void copy_related (const char *inName, const char *outName, 
			  const char *old_ext, const char *new_ext) 
{
  char *in;
  char *out;
  FILE *inFile;
  FILE *outFile;
  int  c;
  size_t name_len = strlen(inName);
  size_t old_len  = strlen(old_ext); 
  size_t new_len  = strlen(new_ext);

  in = malloc(name_len - old_len + new_len + 1);
  strncpy(in, inName, (name_len - old_len));
  strcpy(&in[(name_len - old_len)], new_ext);
#ifdef DEBUG
  printf("DEBUG:copy_related: attempting to open '%s'\n", in);
#endif
  inFile = fopen(in, "rb");
  if (!inFile) {
    free(in);
    return;
  }
  name_len = strlen(outName);
  out = malloc(name_len - old_len + new_len + 1);
  strncpy(out, outName, (name_len - old_len));
  strcpy(&out[(name_len - old_len)], new_ext);
#ifdef DEBUG
  printf("DEBUG:copy_related: attempting to open '%s'\n", out);
#endif
  outFile = fopen(out, "wb");
  if (!out) {
    fprintf(stderr, "%s:%d: couldn't copy related file!\n",
	    __FILE__, __LINE__);
    free(in);
    free(out);
    return;
  }
  while ((c = fgetc(inFile)) != EOF) {
    fputc(c, outFile);
  }
  fclose(inFile);
  fclose(outFile);
  free(in);
  free(out);
}

static char * dupstr (const char *src)
{
  char *dst = malloc(strlen(src) + 1);
  char *cptr;
  if (!dst) {
    fprintf(stderr, "%s:%d: malloc failed!\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }
  cptr = dst;
  while ((*cptr++ = *src++))
    ;
  return dst;
}

struct fieldData {
  int record;
  int null;
  union {
    int i;
    double d;
    char * s;
  } data;
};

typedef int (*compare_function)(const void *, const void *);

#define NUMBER_CMP(_a,_b,_asc)\
do {                          \
  if (!(_a) && !(_b))         \
    return 0;                 \
  if (!(_a) && (_b))          \
    return (_asc) ? -1 : 1;   \
  if ((_a) && !(_b))          \
    return (_asc) ? 1 : -1;   \
  if (*(_a) < *(_b))          \
    return (_asc) ? -1 : 1;   \
  if (*(_a) > *(_b))          \
    return (_asc) ? 1 : -1;   \
  return 0;                   \
} while (0)

static int cmp_int (const int *a, const int *b, int ascending) {
  NUMBER_CMP(a,b,ascending);
}

static int cmp_int_asc(const void *a, const void *b) {
  const struct fieldData *A = a;
  const struct fieldData *B = b;
  return cmp_int ((A->null) ? NULL : &(A->data.i),
		  (B->null) ? NULL : &(B->data.i),
		  1);
}

static int cmp_int_desc(const void *a, const void *b) {
  const struct fieldData *A = a;
  const struct fieldData *B = b;
  return cmp_int ((A->null) ? NULL : &(A->data.i),
		  (B->null) ? NULL : &(B->data.i),
		  0);
}

static int cmp_dbl (const double *a, const double *b, int ascending) {
  NUMBER_CMP(a,b,ascending);
}

static int cmp_dbl_asc(const void *a, const void *b) {
  const struct fieldData *A = a;
  const struct fieldData *B = b;
  return cmp_dbl ((A->null) ? NULL : &(A->data.d),
		  (B->null) ? NULL : &(B->data.d),
		  1);
}

static int cmp_dbl_desc(const void *a, const void *b) {
  const struct fieldData *A = a;
  const struct fieldData *B = b;
  return cmp_dbl ((A->null) ? NULL : &(A->data.d),
		  (B->null) ? NULL : &(B->data.d),
		  0);
}

static int cmp_str(const char *a, const char *b, int ascending) {
  if (!a && !b)
    return 0;
  if (!a && b)
    return (ascending) ? -1 : 1;
  if (a && !b)
    return (ascending) ? 1 : -1;
  return (ascending) ? strcmp(a,b) : strcmp(b,a);
}

static int cmp_str_asc(const void *a, const void *b) {
  const struct fieldData *A = a;
  const struct fieldData *B = b;
  return cmp_str ((A->null) ? NULL : A->data.s,
		  (B->null) ? NULL : B->data.s,
		  1);
}

static int cmp_str_desc(const void *a, const void *b) {
  const struct fieldData *A = a;
  const struct fieldData *B = b;
  return cmp_str ((A->null) ? NULL : A->data.s,
		  (B->null) ? NULL : B->data.s,
		  0);
}


static int * index_dbf (DBFHandle dbf, int field, int ascending)
{
  int *index;
  int width;
  int decimals;
  DBFFieldType fldType;
  char nativeType;
  int count;
  int i;
  struct fieldData *theData;
  compare_function cmp = NULL;

  fldType = DBFGetFieldInfo(dbf, field, NULL, &width, &decimals);
  if (fldType == FTInvalid) {
    fprintf(stderr, "%s:%d: unrecognized field type!\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }
  nativeType = DBFGetNativeFieldType(dbf, field);
  count   = DBFGetRecordCount(dbf);

#ifdef DEBUG
  printf("DEBUG:index_dbf: fldType = %d, nativeType = %c, count = %d\n",
	 fldType, nativeType, count);
#endif

  /* copy data to our struct */
  theData = malloc (sizeof *theData * count);
  if (!theData) {
    fprintf(stderr, "%s:%d: malloc failed!", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }
#ifdef DEBUG
  puts("DEBUG:index_dbf: copying field data.\n");
#endif
  for (i = 0; i < count; i++) {
    theData[i].record = i;
    theData[i].null = DBFIsAttributeNULL(dbf, i, field);
    if (!theData[i].null) {
      switch (fldType) {
      case FTString:
	theData[i].data.s = dupstr(DBFReadStringAttribute(dbf, i, field));
	break;
      case FTInteger:
      case FTLogical:
	theData[i].data.i = DBFReadIntegerAttribute(dbf, i, field);
	break;
      case FTDouble:
	theData[i].data.d = DBFReadDoubleAttribute(dbf, i, field);	
      }
    }
  }

#ifdef DEBUG
    puts("DEBUG:index_dbf: before sort {");
    for (i = 0; i < count; i++) {
      printf ("%5d, %d", theData[i].record, theData[i].null);
      if (!theData[i].null) {
	switch(fldType) {
	case FTString:
	  printf(", %s", theData[i].data.s); break;
	case FTInteger:
	case FTLogical:
	  printf(", %d", theData[i].data.i); break;
	case FTDouble:
	  printf(", %f", theData[i].data.d); break;
	}
      }
      puts("");
    }
    puts("}\n");
#endif

  /* sort the data */
  switch (fldType) {
  case FTString:
    cmp = (ascending) ? cmp_str_asc : cmp_str_desc;
    break;
  case FTInteger:
    cmp = (ascending) ? cmp_int_asc : cmp_int_desc;
    break;
  case FTLogical:
    cmp = (ascending) ? cmp_int_desc : cmp_int_asc;
    break;
  case FTDouble:
    cmp = (ascending) ? cmp_dbl_asc : cmp_dbl_desc;
    break;
  }
  qsort(theData, count, sizeof *theData, cmp);
#ifdef DEBUG
    puts("DEBUG:index_dbf: after sort {");
    for (i = 0; i < count; i++) {
      printf ("%5d, %d", theData[i].record, theData[i].null);
      if (!theData[i].null) {
	switch(fldType) {
	case FTString:
	  printf(", %s", theData[i].data.s); break;
	case FTInteger:
	case FTLogical:
	  printf(", %d", theData[i].data.i); break;
	case FTDouble:
	  printf(", %f", theData[i].data.d); break;
	}
      }
      puts("");
    }
    puts("}\n");
#endif

  /* Build output index */
  index = malloc (sizeof *index * count);
  if (!index) {
    fprintf(stderr, "%s:%d: malloc failed!\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }
  for (i = 0; i < count; i++) {
    index[i] = theData[i].record;
  }

  /* release memory */
  free(theData);

  return index;
}


struct shpdata {
  int record;
  double value;  /* area or length */
};

static int cmp_shpdata_asc (const void *a, const void *b) {
  const struct shpdata *A = a;
  const struct shpdata *B = b;
  return cmp_dbl(&(A->value), &(B->value), 1);
}

static int cmp_shpdata_desc (const void *a, const void *b) {
  const struct shpdata *A = a;
  const struct shpdata *B = b;
  return cmp_dbl(&(A->value), &(B->value), 0);
}

static double area2d_polygon (int n, double *x, double *y) {
  double area = 0;
  int i;
  for (i = 1; i < n; i++) {
    area += (x[i-1] + x[i]) * (y[i] - y[i-1]);
  }
  return area / 2.0;
}

static double shp_area (SHPObject *feat) {
  double area = 0.0;
  if (feat->nParts == 0) {
    area = area2d_polygon (feat->nVertices, feat->padfX, feat->padfY);
  }
  else {
    int part, n;
    for (part = 0; part < feat->nParts; part++) {
      if (part < feat->nParts - 1) {
	n = feat->panPartStart[part+1] - feat->panPartStart[part];
      }
      else {
	n = feat->nVertices - feat->panPartStart[part];
      }
      area += area2d_polygon (n, &(feat->padfX[feat->panPartStart[part]]),
			      &(feat->padfY[feat->panPartStart[part]]));
    }
  }
  /* our area function computes in opposite direction */
  return -area;
}

static double length2d_polyline (int n, double *x, double *y) {
  double length = 0.0;
  int i;
  for (i = 1; i < n; i++) {
    length += sqrt((x[i] - x[i-1])*(x[i] - x[i-1]) 
		   +
		   (y[i] - y[i-1])*(y[i] - y[i-1]));
  }
  return length;
}

static double shp_length (SHPObject *feat) {
  double length = 0.0;
  if (feat->nParts == 0) {
    length = length2d_polyline(feat->nVertices, feat->padfX, feat->padfY);
  }
  else {
    int part, n;
    for (part = 0; part < feat->nParts; part++) {
      if (part < feat->nParts - 1) {
	n = feat->panPartStart[part+1] - feat->panPartStart[part];
      }
      else {
	n = feat->nVertices - feat->panPartStart[part];
      }
      length += length2d_polyline (n, 
				   &(feat->padfX[feat->panPartStart[part]]),
				   &(feat->padfY[feat->panPartStart[part]]));
    }
  }
  return length;
}

static int * index_shape (SHPHandle shp, int ascending)
{
  struct shpdata *data;
  int            *index;
  int             i;
  int             count;
  int             shpType;
  SHPObject       *feat;
  SHPGetInfo(shp, &count, &shpType, NULL, NULL);

#ifdef DEBUG
  printf("DEBUG: index_shape: count = %d, shpType = %d\n", count, shpType);
#endif
 
  switch (shpType) {
  case SHPT_ARC:
  case SHPT_POLYGON:
  case SHPT_ARCZ:
  case SHPT_POLYGONZ:
  case SHPT_ARCM:
  case SHPT_POLYGONM:
    /* actually do something with these */
    break;
  default:
    fprintf(stderr, "Sort is not defined for the Shapefile type!\n");
    exit(EXIT_FAILURE);
  }

  data = malloc(sizeof *data * count);
  if (!data) {
    fprintf(stderr, "%s:%d: malloc failed!\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }
  for (i = 0; i < count; i++) {
    feat = SHPReadObject(shp, i);
    data[i].record = i;
    if (shpType == SHPT_ARC || shpType == SHPT_ARCZ || shpType == SHPT_ARCM) {
      data[i].value = shp_length(feat);
    }
    else {
      data[i].value = shp_area(feat);
    }
  }

#ifdef DEBUG
  puts("DEBUG: index_shape: before sort {");
  for (i = 0; i < count; i++) {
    printf("%5d, %f\n", data[i].record, data[i].value);
  }
  puts("}\n");
#endif

  if (ascending) {
    qsort(data, count, sizeof *data, cmp_shpdata_asc);
  }
  else {
    qsort(data, count, sizeof *data, cmp_shpdata_desc);
  }

#ifdef DEBUG
  puts("DEBUG: index_shape: after sort {");
  for (i = 0; i < count; i++) {
    printf("%5d, %f\n", data[i].record, data[i].value);
  }
  puts("}\n");
#endif
  
  index = malloc(sizeof *index * count);
  if (!index) {
    fprintf(stderr, "%s:%d: malloc failed!\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }
  for (i = 0; i < count; i++) {
    index[i] = data[i].record;
  }

  return index;
}
