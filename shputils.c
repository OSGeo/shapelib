/******************************************************************************
 *
 * Project:  Shapelib
 * Purpose:
 *   Altered "shpdump" and "dbfdump" to allow two files to be appended.
 *   Other Functions:
 *     Selecting from the DBF before the write occurs.
 *     Change the UNITS between Feet and Meters and Shift X,Y.
 *     Clip and Erase boundary.  The program only passes thru the
 *     data once.
 *
 *   Bill Miller   North Carolina - Department of Transportation
 *   Feb. 1997 -- bmiller@dot.state.nc.us
 *         There was not a lot of time to debug hidden problems;
 *         And the code is not very well organized or documented.
 *         The clip/erase function was not well tested.
 *   Oct. 2000 -- bmiller@dot.state.nc.us
 *         Fixed the problem when select is using numbers
 *         larger than short integer.  It now reads long integer.
 *   NOTE: DBF files created using windows NT will read as a string with
 *         a length of 381 characters.  This is a bug in "dbfopen".
 *
 * Author:   Bill Miller (bmiller@dot.state.nc.us)
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
 *
 * SPDX-License-Identifier: MIT OR LGPL-2.0-or-later
 ******************************************************************************
 *
 */

#include "shapefil.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char infile[80], outfile[80], temp[400];

/* Variables for shape files */
SHPHandle hSHP;
SHPHandle hSHPappend;
int nShapeType, nEntities, iPart;
int nShapeTypeAppend, nEntitiesAppend;
SHPObject *psCShape;
double adfBoundsMin[4], adfBoundsMax[4];

/* Variables for DBF files */
DBFHandle hDBF;
DBFHandle hDBFappend;

DBFFieldType iType;
DBFFieldType jType;

char iszTitle[12];
char jszTitle[12];

int *pt;  // TODO(schwehr): Danger.  Shadowed
char iszFormat[32], iszField[1024];
char jszFormat[32], jszField[1024];
int ti, iWidth, iDecimals;
int tj, jWidth, jDecimals;

/* -------------------------------------------------------------------- */
/* Variables for the DESCRIBE function */
/* -------------------------------------------------------------------- */
bool ilist = false;
bool iall = false;
/* -------------------------------------------------------------------- */
/* Variables for the SELECT function */
/* -------------------------------------------------------------------- */
bool found = false;
bool newdbf = false;
char selectitem[40], *cpt;
long int selectvalues[150], selcount = 0;
bool iselect = false;
int iselectitem = -1;
bool iunselect = false;

/* -------------------------------------------------------------------- */
/* Variables for the CLIP and ERASE functions */
/* -------------------------------------------------------------------- */
double cxmin, cymin, cxmax, cymax;
bool iclip = false;
bool ierase = false;
bool itouch = false;
bool iinside = false;
bool icut = false;
char clipfile[80];

/* -------------------------------------------------------------------- */
/* Variables for the FACTOR function */
/* -------------------------------------------------------------------- */
double infactor, outfactor, factor = 0; /* NO FACTOR */
bool iunit = false;

/* -------------------------------------------------------------------- */
/* Variables for the SHIFT function */
/* -------------------------------------------------------------------- */
double xshift = 0, yshift = 0; /* NO SHIFT */

/* -------------------------------------------------------------------- */
/*	Change the extension.  If there is any extension on the 	*/
/*	filename, strip it off and add the new extension                */
/* -------------------------------------------------------------------- */
void setext(char *pt, const char *ext)
{
    int i = (int)(strlen(pt) - 1);
    for (; i > 0 && pt[i] != '.' && pt[i] != '/' && pt[i] != '\\'; i--)
    {
    }

    if (pt[i] == '.')
        pt[i] = '\0';

    strcat(pt, ".");
    strcat(pt, ext);
}

/************************************************************************/
/*                             openfiles()                              */
/************************************************************************/

void openfiles()
{
    /* -------------------------------------------------------------------- */
    /*      Open the DBF file.                                              */
    /* -------------------------------------------------------------------- */
    setext(infile, "dbf");
    hDBF = DBFOpen(infile, "rb");
    if (hDBF == NULL)
    {
        printf("ERROR: Unable to open the input DBF:%s\n", infile);
        exit(1);
    }
    /* -------------------------------------------------------------------- */
    /*      Open the append DBF file.                                       */
    /* -------------------------------------------------------------------- */
    if (strcmp(outfile, ""))
    {
        setext(outfile, "dbf");
        hDBFappend = DBFOpen(outfile, "rb+");
        newdbf = false;
        if (hDBFappend == NULL)
        {
            newdbf = true;
            hDBFappend = DBFCreate(outfile);
            if (hDBFappend == NULL)
            {
                printf("ERROR: Unable to open the append DBF:%s\n", outfile);
                exit(1);
            }
        }
    }
    /* -------------------------------------------------------------------- */
    /*      Open the passed shapefile.                                      */
    /* -------------------------------------------------------------------- */
    setext(infile, "shp");
    hSHP = SHPOpen(infile, "rb");

    if (hSHP == NULL)
    {
        printf("ERROR: Unable to open the input shape file:%s\n", infile);
        exit(1);
    }

    SHPGetInfo(hSHP, &nEntities, &nShapeType, NULL, NULL);

    /* -------------------------------------------------------------------- */
    /*      Open the passed append shapefile.                               */
    /* -------------------------------------------------------------------- */
    if (strcmp(outfile, ""))
    {
        setext(outfile, "shp");
        hSHPappend = SHPOpen(outfile, "rb+");

        if (hSHPappend == NULL)
        {
            hSHPappend = SHPCreate(outfile, nShapeType);
            if (hSHPappend == NULL)
            {
                printf("ERROR: Unable to open the append shape file:%s\n",
                       outfile);
                exit(1);
            }
        }
        SHPGetInfo(hSHPappend, &nEntitiesAppend, &nShapeTypeAppend, NULL, NULL);

        if (nShapeType != nShapeTypeAppend)
        {
            puts("ERROR: Input and Append shape files are of different types.");
            exit(1);
        }
    }
}

/* -------------------------------------------------------------------- */
/*	Find matching fields in the append file.                        */
/*      Output file must have zero records to add any new fields.       */
/* -------------------------------------------------------------------- */
void mergefields()
{
    ti = DBFGetFieldCount(hDBF);
    tj = DBFGetFieldCount(hDBFappend);
    /* Create a pointer array for the max # of fields in the output file */
    pt = (int *)malloc((ti + tj + 1) * sizeof(int));

    for (int i = 0; i < ti; i++)
    {
        pt[i] = -1; /* Initial pt values to -1 */
    }
    /* DBF must be empty before adding items */
    const int jRecord = DBFGetRecordCount(hDBFappend);
    int j;
    for (int i = 0; i < ti; i++)
    {
        iType = DBFGetFieldInfo(hDBF, i, iszTitle, &iWidth, &iDecimals);
        found = false;

        for (j = 0; j < tj; j++) /* Search all field names for a match */
        {
            jType =
                DBFGetFieldInfo(hDBFappend, j, jszTitle, &jWidth, &jDecimals);
            if (iType == jType && (strcmp(iszTitle, jszTitle) == 0))
            {
                if (found || newdbf)
                {
                    if (i == j)
                        pt[i] = j;
                    printf("Warning: Duplicate field name found (%s)\n",
                           iszTitle);
                    /* Duplicate field name
	               (Try to guess the correct field by position) */
                }
                else
                {
                    pt[i] = j;
                    found = true;
                }
            }
        }

        if (pt[i] == -1 && (!found)) /* Try to force into an existing field */
        { /* Ignore the field name, width, and decimal places */
            jType =
                DBFGetFieldInfo(hDBFappend, j, jszTitle, &jWidth, &jDecimals);
            if (iType == jType)
            {
                pt[i] = i;
                found = true;
            }
        }
        if ((!found) &&
            jRecord == 0) /* Add missing field to the append table */
        {                 /* The output DBF must be is empty */
            pt[i] = tj;
            tj++;
            if (DBFAddField(hDBFappend, iszTitle, iType, iWidth, iDecimals) ==
                -1)
            {
                printf("Warning: DBFAddField(%s, TYPE:%d, WIDTH:%d  DEC:%d, "
                       "ITEM#:%d of %d) failed.\n",
                       iszTitle, iType, iWidth, iDecimals, (i + 1), (ti + 1));
                pt[i] = -1;
            }
        }
    }
}

/************************************************************************/
/*                            strncasecmp2()                            */
/*                                                                      */
/*      Compare two strings up to n characters                          */
/*      If n=0 then s1 and s2 must be an exact match                    */
/************************************************************************/
int strncasecmp2(char *s1, char *s2, int n)
{
    if (n < 1)
        n = (int)(strlen(s1) + 1);

    for (int i = 0; i < n; i++)
    {
        if (*s1 != *s2)
        {
            if (*s1 >= 'a' && *s1 <= 'z')
            {
                const int j = *s1 - 32;
                if (j != *s2)
                    return (*s1 - *s2);
            }
            else
            {
                int j;
                if (*s1 >= 'A' && *s1 <= 'Z')
                {
                    j = *s1 + 32;
                }
                else
                {
                    j = *s1;
                }
                if (j != *s2)
                    return (*s1 - *s2);
            }
        }
        s1++;
        s2++;
    }
    return (0);
}

void showitems()
{
    printf("Available Items: (%d)", ti);
    long int maxrec = DBFGetRecordCount(hDBF);
    if (maxrec > 5000 && !iall)
    {
        maxrec = 5000;
        printf("  ** ESTIMATED RANGES (MEAN)  For more records use \"All\"");
    }
    else
    {
        printf("          RANGES (MEAN)");
    }

    char stmp[40] = {0};
    char slow[40] = {0};
    char shigh[40] = {0};

    for (int i = 0; i < ti; i++)
    {
        switch (DBFGetFieldInfo(hDBF, i, iszTitle, &iWidth, &iDecimals))
        {
            case FTString:
            case FTLogical:
            case FTDate:
                strcpy(slow, "~");
                strcpy(shigh, "\0");
                printf("\n  String  %3d  %-16s", iWidth, iszTitle);
                for (int iRecord = 0; iRecord < maxrec; iRecord++)
                {
                    strncpy(stmp, DBFReadStringAttribute(hDBF, iRecord, i), 39);
                    if (strcmp(stmp, "!!") > 0)
                    {
                        if (strncasecmp2(stmp, slow, 0) < 0)
                            memcpy(slow, stmp, 39);
                        if (strncasecmp2(stmp, shigh, 0) > 0)
                            memcpy(shigh, stmp, 39);
                    }
                }
                char *pt = slow + strlen(slow) - 1;
                while (*pt == ' ')
                {
                    *pt = '\0';
                    pt--;
                }
                pt = shigh + strlen(shigh) - 1;
                while (*pt == ' ')
                {
                    *pt = '\0';
                    pt--;
                }
                if (strncasecmp2(slow, shigh, 0) < 0)
                    printf("%s to %s", slow, shigh);
                else if (strncasecmp2(slow, shigh, 0) == 0)
                    printf("= %s", slow);
                else
                    printf("No Values");
                break;
            case FTInteger:
            {
                printf("\n  Integer %3d  %-16s", iWidth, iszTitle);
                long int ilow = 1999999999;
                long int ihigh = -1999999999;
                long int isum = 0;
                for (int iRecord = 0; iRecord < maxrec; iRecord++)
                {
                    const long int itmp =
                        DBFReadIntegerAttribute(hDBF, iRecord, i);
                    if (ilow > itmp)
                        ilow = itmp;
                    if (ihigh < itmp)
                        ihigh = itmp;
                    isum = isum + itmp;
                }
                const double mean = isum / maxrec;
                if (ilow < ihigh)
                    printf("%ld to %ld \t(%.1f)", ilow, ihigh, mean);
                else if (ilow == ihigh)
                    printf("= %ld", ilow);
                else
                    printf("No Values");
                break;
            }
            case FTDouble:
            {
                printf("\n  Real  %3d,%d  %-16s", iWidth, iDecimals, iszTitle);
                double dlow = 999999999999999.0;
                double dhigh = -999999999999999.0;
                double dsum = 0;
                for (int iRecord = 0; iRecord < maxrec; iRecord++)
                {
                    const double dtmp =
                        DBFReadDoubleAttribute(hDBF, iRecord, i);
                    if (dlow > dtmp)
                        dlow = dtmp;
                    if (dhigh < dtmp)
                        dhigh = dtmp;
                    dsum = dsum + dtmp;
                }
                const double mean = dsum / maxrec;
                sprintf(stmp, "%%.%df to %%.%df \t(%%.%df)", iDecimals,
                        iDecimals, iDecimals);
                if (dlow < dhigh)
                    printf(stmp, dlow, dhigh, mean);
                else if (dlow == dhigh)
                {
                    sprintf(stmp, "= %%.%df", iDecimals);
                    printf(stmp, dlow);
                }
                else
                    printf("No Values");
                break;
            }
            case FTInvalid:
                break;
        }
    }
    printf("\n");
}

void findselect()
{
    /* Find the select field name */
    iselectitem = -1;
    for (int i = 0; i < ti && iselectitem < 0; i++)
    {
        iType = DBFGetFieldInfo(hDBF, i, iszTitle, &iWidth, &iDecimals);
        if (strncasecmp2(iszTitle, selectitem, 0) == 0)
            iselectitem = i;
    }
    if (iselectitem == -1)
    {
        printf("Warning: Item not found for selection (%s)\n", selectitem);
        iselect = false;
        iall = false;
        showitems();
        printf("Continued... (Selecting entire file)\n");
    }
    /* Extract all of the select values (by field type) */
}

int selectrec(int iRecord)
{
    const long int ty =
        DBFGetFieldInfo(hDBF, iselectitem, NULL, &iWidth, &iDecimals);
    switch (ty)
    {
        case FTString:
            puts("Invalid Item");
            iselect = false;
            break;
        case FTInteger:
        {
            const long int value =
                DBFReadIntegerAttribute(hDBF, iRecord, iselectitem);
            for (int j = 0; j < selcount; j++)
            {
                if (selectvalues[j] == value)
                {
                    if (iunselect)
                        return (0); /* Keep this record */
                    else
                        return (1); /* Skip this record */
                }
            }
            break;
        }
        case FTDouble:
            puts("Invalid Item");
            iselect = false;
            break;
    }
    if (iunselect)
        return (1); /* Skip this record */
    else
        return (0); /* Keep this record */
}

void check_theme_bnd()
{
    if ((adfBoundsMin[0] >= cxmin) && (adfBoundsMax[0] <= cxmax) &&
        (adfBoundsMin[1] >= cymin) && (adfBoundsMax[1] <= cymax))
    { /** Theme is totally inside clip area **/
        if (ierase)
            nEntities = 0; /** SKIP THEME  **/
        else
            iclip = false; /** WRITE THEME (Clip not needed) **/
    }

    if (((adfBoundsMin[0] < cxmin) && (adfBoundsMax[0] < cxmin)) ||
        ((adfBoundsMin[1] < cymin) && (adfBoundsMax[1] < cymin)) ||
        ((adfBoundsMin[0] > cxmax) && (adfBoundsMax[0] > cxmax)) ||
        ((adfBoundsMin[1] > cymax) && (adfBoundsMax[1] > cymax)))
    { /** Theme is totally outside clip area **/
        if (ierase)
            iclip = false; /** WRITE THEME (Clip not needed) **/
        else
            nEntities = 0; /** SKIP THEME  **/
    }

    if (nEntities == 0)
        puts("WARNING: Theme is outside the clip area."); /** SKIP THEME  **/
}

/* -------------------------------------------------------------------- */
/*      Compute where a line segment (x0,y0)-(x1,y1) first crosses     */
/*      the axis-aligned clip rectangle.  Returns true if an            */
/*      intersection was found, storing it in (*xi, *yi).               */
/* -------------------------------------------------------------------- */
bool compute_clip_intersection(double x0, double y0, double x1, double y1,
                               double clipxmin, double clipymin,
                               double clipxmax, double clipymax, double *xi,
                               double *yi)
{
    const double dx = x1 - x0;
    const double dy = y1 - y0;
    double tmin = 2.0; /* > 1 means no valid intersection yet */
    double best_xi = 0, best_yi = 0;

    /* Left edge (x = clipxmin) */
    if (dx != 0.0)
    {
        const double t = (clipxmin - x0) / dx;
        if (t > 0.0 && t < 1.0)
        {
            const double y = y0 + t * dy;
            if (y >= clipymin && y <= clipymax && t < tmin)
            {
                tmin = t;
                best_xi = clipxmin;
                best_yi = y;
            }
        }
    }

    /* Right edge (x = clipxmax) */
    if (dx != 0.0)
    {
        const double t = (clipxmax - x0) / dx;
        if (t > 0.0 && t < 1.0)
        {
            const double y = y0 + t * dy;
            if (y >= clipymin && y <= clipymax && t < tmin)
            {
                tmin = t;
                best_xi = clipxmax;
                best_yi = y;
            }
        }
    }

    /* Bottom edge (y = clipymin) */
    if (dy != 0.0)
    {
        const double t = (clipymin - y0) / dy;
        if (t > 0.0 && t < 1.0)
        {
            const double x = x0 + t * dx;
            if (x >= clipxmin && x <= clipxmax && t < tmin)
            {
                tmin = t;
                best_xi = x;
                best_yi = clipymin;
            }
        }
    }

    /* Top edge (y = clipymax) */
    if (dy != 0.0)
    {
        const double t = (clipymax - y0) / dy;
        if (t > 0.0 && t < 1.0)
        {
            const double x = x0 + t * dx;
            if (x >= clipxmin && x <= clipxmax && t < tmin)
            {
                tmin = t;
                best_xi = x;
                best_yi = clipymax;
            }
        }
    }

    if (tmin <= 1.0)
    {
        *xi = best_xi;
        *yi = best_yi;
        return true;
    }
    return false;
}

/* -------------------------------------------------------------------- */
/*      Sutherland-Hodgman polygon clipping against a single edge.      */
/*      edge_code: 0=left, 1=right, 2=bottom, 3=top                    */
/*      Returns the new vertex count.                                   */
/* -------------------------------------------------------------------- */
static int sh_clip_edge(const double *inX, const double *inY, const double *inZ,
                        const double *inM, int nIn, double *outX, double *outY,
                        double *outZ, double *outM, int edge_code,
                        double edge_val)
{
    int nOut = 0;
    for (int i = 0; i < nIn; i++)
    {
        const int j = (i + 1) % nIn;
        double si, sj; /* signed distance to edge */

        switch (edge_code)
        {
            case 0:
                si = inX[i] - edge_val;
                sj = inX[j] - edge_val;
                break; /* left:  inside when x >= val */
            case 1:
                si = edge_val - inX[i];
                sj = edge_val - inX[j];
                break; /* right: inside when x <= val */
            case 2:
                si = inY[i] - edge_val;
                sj = inY[j] - edge_val;
                break; /* bottom: inside when y >= val */
            case 3:
                si = edge_val - inY[i];
                sj = edge_val - inY[j];
                break; /* top: inside when y <= val */
            default:
                return 0;
        }

        const bool i_in = (si >= 0);
        const bool j_in = (sj >= 0);

        if (i_in && j_in)
        {
            /* Both inside -> output j */
            outX[nOut] = inX[j];
            outY[nOut] = inY[j];
            if (outZ)
                outZ[nOut] = inZ[j];
            if (outM)
                outM[nOut] = inM[j];
            nOut++;
        }
        else if (i_in && !j_in)
        {
            /* Leaving -> output intersection */
            const double t = si / (si - sj);
            outX[nOut] = inX[i] + t * (inX[j] - inX[i]);
            outY[nOut] = inY[i] + t * (inY[j] - inY[i]);
            if (outZ)
                outZ[nOut] = inZ[i] + t * (inZ[j] - inZ[i]);
            if (outM)
                outM[nOut] = inM[i] + t * (inM[j] - inM[i]);
            nOut++;
        }
        else if (!i_in && j_in)
        {
            /* Entering -> output intersection, then j */
            const double t = si / (si - sj);
            outX[nOut] = inX[i] + t * (inX[j] - inX[i]);
            outY[nOut] = inY[i] + t * (inY[j] - inY[i]);
            if (outZ)
                outZ[nOut] = inZ[i] + t * (inZ[j] - inZ[i]);
            if (outM)
                outM[nOut] = inM[i] + t * (inM[j] - inM[i]);
            nOut++;
            outX[nOut] = inX[j];
            outY[nOut] = inY[j];
            if (outZ)
                outZ[nOut] = inZ[j];
            if (outM)
                outM[nOut] = inM[j];
            nOut++;
        }
        /* else both outside -> nothing */
    }
    return nOut;
}

int clip_boundary()
{
    /*** FIRST check the boundary of the feature ***/
    if (((psCShape->dfXMin < cxmin) && (psCShape->dfXMax < cxmin)) ||
        ((psCShape->dfYMin < cymin) && (psCShape->dfYMax < cymin)) ||
        ((psCShape->dfXMin > cxmax) && (psCShape->dfXMax > cxmax)) ||
        ((psCShape->dfYMin > cymax) && (psCShape->dfYMax > cymax)))
    { /** Feature is totally outside clip area **/
        if (ierase)
            return (1); /** WRITE RECORD **/
        else
            return (0); /** SKIP  RECORD **/
    }

    if ((psCShape->dfXMin >= cxmin) && (psCShape->dfXMax <= cxmax) &&
        (psCShape->dfYMin >= cymin) && (psCShape->dfYMax <= cymax))
    { /** Feature is totally inside clip area **/
        if (ierase)
            return (0); /** SKIP  RECORD **/
        else
            return (1); /** WRITE RECORD **/
    }

    if (iinside)
    { /** INSIDE * Feature might touch the boundary or could be outside **/
        if (ierase)
            return (1); /** WRITE RECORD **/
        else
            return (0); /** SKIP  RECORD **/
    }

    if (itouch)
    { /** TOUCH **/
        if (((psCShape->dfXMin <= cxmin) || (psCShape->dfXMax >= cxmax)) &&
            (psCShape->dfYMin >= cymin) && (psCShape->dfYMax <= cymax))
        { /** Feature intersects the clip boundary only on the X axis **/
            if (ierase)
                return (0); /** SKIP  RECORD **/
            else
                return (1); /** WRITE RECORD **/
        }

        if ((psCShape->dfXMin >= cxmin) && (psCShape->dfXMax <= cxmax) &&
            ((psCShape->dfYMin <= cymin) || (psCShape->dfYMax >= cymax)))
        { /** Feature intersects the clip boundary only on the Y axis **/
            if (ierase)
                return (0); /** SKIP  RECORD **/
            else
                return (1); /** WRITE RECORD **/
        }

        for (int j2 = 0; j2 < psCShape->nVertices; j2++)
        { /** At least one vertex must be inside the clip boundary **/
            if ((psCShape->padfX[j2] >= cxmin &&
                 psCShape->padfX[j2] <= cxmax) ||
                (psCShape->padfY[j2] >= cymin && psCShape->padfY[j2] <= cymax))
            {
                if (ierase)
                    return (0); /** SKIP  RECORD **/
                else
                    return (1); /** WRITE RECORD **/
            }
        }

        /** All vertices are outside the clip boundary **/
        if (ierase)
            return (1); /** WRITE RECORD **/
        else
            return (0); /** SKIP  RECORD **/
    } /** End TOUCH **/

    if (icut)
    { /** CUT **/
        const bool is_polygon = (psCShape->nSHPType == SHPT_POLYGON ||
                                 psCShape->nSHPType == SHPT_POLYGONZ ||
                                 psCShape->nSHPType == SHPT_POLYGONM);

        if (is_polygon && !ierase)
        {
            const int nParts = psCShape->nParts > 0 ? psCShape->nParts : 1;
            const int totalOrig = psCShape->nVertices;
            /* Worst case: each ring gains a few vertices from clipping */
            const int maxTotal = totalOrig * 3 + nParts * 10;

            double *outX = (double *)malloc(maxTotal * sizeof(double));
            double *outY = (double *)malloc(maxTotal * sizeof(double));
            double *outZ = psCShape->padfZ
                               ? (double *)malloc(maxTotal * sizeof(double))
                               : NULL;
            double *outM = psCShape->padfM
                               ? (double *)malloc(maxTotal * sizeof(double))
                               : NULL;
            int *outParts = (int *)malloc(nParts * sizeof(int));
            int outNParts = 0;
            int outTotal = 0;

            for (int p = 0; p < nParts; p++)
            {
                /* Determine ring range */
                const int ringStart =
                    psCShape->panPartStart ? psCShape->panPartStart[p] : 0;
                const int ringEnd = (p + 1 < nParts && psCShape->panPartStart)
                                        ? psCShape->panPartStart[p + 1]
                                        : totalOrig;
                int nRing = ringEnd - ringStart;

                /* Strip closing vertex if present */
                if (nRing >= 4 &&
                    psCShape->padfX[ringStart] ==
                        psCShape->padfX[ringStart + nRing - 1] &&
                    psCShape->padfY[ringStart] ==
                        psCShape->padfY[ringStart + nRing - 1])
                    nRing--;

                if (nRing < 3)
                    continue;

                /* Allocate SH ping-pong buffers for this ring */
                const int maxRing = (nRing + 4) * 2 + 2;
                double *bufX[2], *bufY[2], *bufZ[2], *bufM[2];
                for (int b = 0; b < 2; b++)
                {
                    bufX[b] = (double *)malloc(maxRing * sizeof(double));
                    bufY[b] = (double *)malloc(maxRing * sizeof(double));
                    bufZ[b] = psCShape->padfZ
                                  ? (double *)malloc(maxRing * sizeof(double))
                                  : NULL;
                    bufM[b] = psCShape->padfM
                                  ? (double *)malloc(maxRing * sizeof(double))
                                  : NULL;
                }

                memcpy(bufX[0], psCShape->padfX + ringStart,
                       nRing * sizeof(double));
                memcpy(bufY[0], psCShape->padfY + ringStart,
                       nRing * sizeof(double));
                if (psCShape->padfZ)
                    memcpy(bufZ[0], psCShape->padfZ + ringStart,
                           nRing * sizeof(double));
                if (psCShape->padfM)
                    memcpy(bufM[0], psCShape->padfM + ringStart,
                           nRing * sizeof(double));

                int nCur = nRing;
                int src = 0;

                /* Clip against 4 edges: left, right, bottom, top */
                const int edges[] = {0, 1, 2, 3};
                const double vals[] = {cxmin, cxmax, cymin, cymax};
                for (int e = 0; e < 4 && nCur > 0; e++)
                {
                    const int dst = 1 - src;
                    nCur =
                        sh_clip_edge(bufX[src], bufY[src], bufZ[src], bufM[src],
                                     nCur, bufX[dst], bufY[dst], bufZ[dst],
                                     bufM[dst], edges[e], vals[e]);
                    src = dst;
                }

                if (nCur >= 3)
                {
                    /* Record part start */
                    outParts[outNParts++] = outTotal;

                    /* Copy clipped ring + closing vertex */
                    memcpy(outX + outTotal, bufX[src], nCur * sizeof(double));
                    memcpy(outY + outTotal, bufY[src], nCur * sizeof(double));
                    if (outZ)
                        memcpy(outZ + outTotal, bufZ[src],
                               nCur * sizeof(double));
                    if (outM)
                        memcpy(outM + outTotal, bufM[src],
                               nCur * sizeof(double));
                    outTotal += nCur;

                    /* Close the ring */
                    outX[outTotal] = outX[outTotal - nCur];
                    outY[outTotal] = outY[outTotal - nCur];
                    if (outZ)
                        outZ[outTotal] = outZ[outTotal - nCur];
                    if (outM)
                        outM[outTotal] = outM[outTotal - nCur];
                    outTotal++;
                }

                for (int b = 0; b < 2; b++)
                {
                    free(bufX[b]);
                    free(bufY[b]);
                    free(bufZ[b]);
                    free(bufM[b]);
                }
            }

            if (outNParts > 0)
            {
                free(psCShape->padfX);
                free(psCShape->padfY);
                psCShape->padfX = outX;
                psCShape->padfY = outY;
                if (psCShape->padfZ)
                {
                    free(psCShape->padfZ);
                    psCShape->padfZ = outZ;
                }
                if (psCShape->padfM)
                {
                    free(psCShape->padfM);
                    psCShape->padfM = outM;
                }
                free(psCShape->panPartStart);
                psCShape->panPartStart = outParts;
                psCShape->nParts = outNParts;
                psCShape->nVertices = outTotal;
                return (1); /** WRITE RECORD **/
            }

            free(outX);
            free(outY);
            free(outZ);
            free(outM);
            free(outParts);
            return (0); /** SKIP RECORD **/
        }

        const int nOrig = psCShape->nVertices;
        const int maxVerts = nOrig * 3 + 2;
        double *newX = (double *)malloc(maxVerts * sizeof(double));
        double *newY = (double *)malloc(maxVerts * sizeof(double));
        double *newZ = psCShape->padfZ
                           ? (double *)malloc(maxVerts * sizeof(double))
                           : NULL;
        double *newM = psCShape->padfM
                           ? (double *)malloc(maxVerts * sizeof(double))
                           : NULL;
        int i2 = 0;

        for (int j2 = 0; j2 < nOrig; j2++)
        {
            bool cur_inside =
                psCShape->padfX[j2] >= cxmin && psCShape->padfX[j2] <= cxmax &&
                psCShape->padfY[j2] >= cymin && psCShape->padfY[j2] <= cymax;
            if (ierase)
                cur_inside = !cur_inside;

            if (j2 > 0)
            {
                bool prev_inside = psCShape->padfX[j2 - 1] >= cxmin &&
                                   psCShape->padfX[j2 - 1] <= cxmax &&
                                   psCShape->padfY[j2 - 1] >= cymin &&
                                   psCShape->padfY[j2 - 1] <= cymax;
                if (ierase)
                    prev_inside = !prev_inside;

                if (cur_inside != prev_inside)
                {
                    double xi, yi;
                    if (compute_clip_intersection(
                            psCShape->padfX[j2 - 1], psCShape->padfY[j2 - 1],
                            psCShape->padfX[j2], psCShape->padfY[j2], cxmin,
                            cymin, cxmax, cymax, &xi, &yi))
                    {
                        newX[i2] = xi;
                        newY[i2] = yi;
                        if (newZ || newM)
                        {
                            const double dx =
                                psCShape->padfX[j2] - psCShape->padfX[j2 - 1];
                            const double dy =
                                psCShape->padfY[j2] - psCShape->padfY[j2 - 1];
                            const double t =
                                (fabs(dx) > fabs(dy))
                                    ? (xi - psCShape->padfX[j2 - 1]) / dx
                                    : (yi - psCShape->padfY[j2 - 1]) / dy;
                            if (newZ)
                                newZ[i2] = psCShape->padfZ[j2 - 1] +
                                           t * (psCShape->padfZ[j2] -
                                                psCShape->padfZ[j2 - 1]);
                            if (newM)
                                newM[i2] = psCShape->padfM[j2 - 1] +
                                           t * (psCShape->padfM[j2] -
                                                psCShape->padfM[j2 - 1]);
                        }
                        i2++;
                    }
                }
                else if (!cur_inside && !prev_inside)
                {
                    /* Both outside: check if segment crosses through box */
                    double xi1, yi1, xi2, yi2;
                    if (compute_clip_intersection(
                            psCShape->padfX[j2 - 1], psCShape->padfY[j2 - 1],
                            psCShape->padfX[j2], psCShape->padfY[j2], cxmin,
                            cymin, cxmax, cymax, &xi1, &yi1) &&
                        compute_clip_intersection(
                            psCShape->padfX[j2], psCShape->padfY[j2],
                            psCShape->padfX[j2 - 1], psCShape->padfY[j2 - 1],
                            cxmin, cymin, cxmax, cymax, &xi2, &yi2))
                    {
                        newX[i2] = xi1;
                        newY[i2] = yi1;
                        if (newZ || newM)
                        {
                            const double dx =
                                psCShape->padfX[j2] - psCShape->padfX[j2 - 1];
                            const double dy =
                                psCShape->padfY[j2] - psCShape->padfY[j2 - 1];
                            const double t =
                                (fabs(dx) > fabs(dy))
                                    ? (xi1 - psCShape->padfX[j2 - 1]) / dx
                                    : (yi1 - psCShape->padfY[j2 - 1]) / dy;
                            if (newZ)
                                newZ[i2] = psCShape->padfZ[j2 - 1] +
                                           t * (psCShape->padfZ[j2] -
                                                psCShape->padfZ[j2 - 1]);
                            if (newM)
                                newM[i2] = psCShape->padfM[j2 - 1] +
                                           t * (psCShape->padfM[j2] -
                                                psCShape->padfM[j2 - 1]);
                        }
                        i2++;
                        newX[i2] = xi2;
                        newY[i2] = yi2;
                        if (newZ || newM)
                        {
                            const double dx =
                                psCShape->padfX[j2] - psCShape->padfX[j2 - 1];
                            const double dy =
                                psCShape->padfY[j2] - psCShape->padfY[j2 - 1];
                            const double t =
                                (fabs(dx) > fabs(dy))
                                    ? (xi2 - psCShape->padfX[j2 - 1]) / dx
                                    : (yi2 - psCShape->padfY[j2 - 1]) / dy;
                            if (newZ)
                                newZ[i2] = psCShape->padfZ[j2 - 1] +
                                           t * (psCShape->padfZ[j2] -
                                                psCShape->padfZ[j2 - 1]);
                            if (newM)
                                newM[i2] = psCShape->padfM[j2 - 1] +
                                           t * (psCShape->padfM[j2] -
                                                psCShape->padfM[j2 - 1]);
                        }
                        i2++;
                    }
                }
            }

            if (cur_inside)
            {
                newX[i2] = psCShape->padfX[j2];
                newY[i2] = psCShape->padfY[j2];
                if (newZ)
                    newZ[i2] = psCShape->padfZ[j2];
                if (newM)
                    newM[i2] = psCShape->padfM[j2];
                i2++;
            }
        }

        free(psCShape->padfX);
        free(psCShape->padfY);
        psCShape->padfX = newX;
        psCShape->padfY = newY;
        if (psCShape->padfZ)
        {
            free(psCShape->padfZ);
            psCShape->padfZ = newZ;
        }
        if (psCShape->padfM)
        {
            free(psCShape->padfM);
            psCShape->padfM = newM;
        }
        psCShape->nVertices = i2;

        if (i2 < 2)
            return (0); /** SKIP RECORD **/

        return (1); /** WRITE RECORD **/
    } /** End CUT **/

    return 0;
}

#define NKEYS (sizeof(unitkeytab) / sizeof(struct unitkey))
double findunit(char *unit)
{
    struct unitkey
    {
        char *name;
        double value;
    } unitkeytab[] = {{"CM", 39.37},           {"CENTIMETER", 39.37},
                      {"CENTIMETERS", 39.37}, /** # of inches * 100 in unit **/
                      {"METER", 3937},         {"METERS", 3937},
                      {"KM", 3937000},         {"KILOMETER", 3937000},
                      {"KILOMETERS", 3937000}, {"INCH", 100},
                      {"INCHES", 100},         {"FEET", 1200},
                      {"FOOT", 1200},          {"YARD", 3600},
                      {"YARDS", 3600},         {"MILE", 6336000},
                      {"MILES", 6336000}};

    double unitfactor = 0;
    for (int j = 0; j < (int)NKEYS; j++)
    {
        if (strncasecmp2(unit, unitkeytab[j].name, 0) == 0)
            unitfactor = unitkeytab[j].value;
    }
    return (unitfactor);
}

/* -------------------------------------------------------------------- */
/*      Parse comma-separated integer selection values from a string.   */
/*      Returns the number of values parsed.                            */
/* -------------------------------------------------------------------- */
long int parse_select_values(const char *input, long int *values,
                             int max_values)
{
    long int count = 0;
    const char *cp = input;
    long int val = atol(cp);
    while (val > 0 && count < max_values)
    {
        values[count] = val;
        while (*cp >= '0' && *cp <= '9')
            cp++;
        while (*cp > '\0' && (*cp < '0' || *cp > '9'))
            cp++;
        val = atol(cp);
        count++;
    }
    return count;
}

/* -------------------------------------------------------------------- */
/*      Apply factor and shift to the coordinates of a shape object.    */
/* -------------------------------------------------------------------- */
void transform_coordinates(SHPObject *psShape, double dfFactor, double dfXShift,
                           double dfYShift)
{
    for (int j = 0; j < psShape->nVertices; j++)
    {
        psShape->padfX[j] = psShape->padfX[j] * dfFactor + dfXShift;
        psShape->padfY[j] = psShape->padfY[j] * dfFactor + dfYShift;
    }
}

/* -------------------------------------------------------------------- */
/*      Copy one DBF record from hDBFin(iRecord) to hDBFout(jRecord)    */
/*      using the field mapping in fieldmap[].                          */
/*      Returns true on success, false if any write fails.              */
/* -------------------------------------------------------------------- */
bool copy_dbf_record(DBFHandle hDBFin, int iRecord, DBFHandle hDBFout,
                     int jRecord, const int *fieldmap, int nFields)
{
    int w, d;
    for (int i = 0; i < nFields; i++)
    {
        if (fieldmap[i] < 0)
            continue;
        switch (DBFGetFieldInfo(hDBFin, i, NULL, &w, &d))
        {
            case FTString:
            case FTLogical:
            case FTDate:
            {
                const char *val = DBFReadStringAttribute(hDBFin, iRecord, i);
                if (val == NULL)
                    return false;
                if (!DBFWriteStringAttribute(hDBFout, jRecord, fieldmap[i],
                                             val))
                    return false;
                break;
            }
            case FTInteger:
                if (!DBFWriteIntegerAttribute(
                        hDBFout, jRecord, fieldmap[i],
                        DBFReadIntegerAttribute(hDBFin, iRecord, i)))
                    return false;
                break;
            case FTDouble:
                if (!DBFWriteDoubleAttribute(
                        hDBFout, jRecord, fieldmap[i],
                        DBFReadDoubleAttribute(hDBFin, iRecord, i)))
                    return false;
                break;
            case FTInvalid:
                break;
        }
    }
    return true;
}

/* -------------------------------------------------------------------- */
/*      Display a usage message.                                        */
/* -------------------------------------------------------------------- */
void error()
{
    puts("The program will append to an existing shape file or it will");
    puts("create a new file if needed.");
    puts("Only the items in the first output file will be preserved.");
    puts("When an item does not match with the append theme then the item");
    puts("might be placed to an existing item at the same position and type.");
    puts("  OTHER FUNCTIONS:");
    puts("  - Describe all items in the dbase file (Use ALL for more than 5000 "
         "recs.)");
    puts("  - Select a group of shapes from a comma separated selection list.");
    puts("  - UnSelect a group of shapes from a comma separated selection "
         "list.");
    puts("  - Clip boundary extent or by theme boundary.");
    puts("      Touch writes all the shapes that touch the boundary.");
    puts("      Inside writes all the shapes that are completely within the "
         "boundary.");
    puts("      Boundary clips are only the min and max of a theme boundary.");
    puts("  - Erase boundary extent or by theme boundary.");
    puts("      Erase is the direct opposite of the Clip function.");
    puts("  - Change coordinate value units between meters and feet.");
    puts("      There is no way to determine the input unit of a shape file.");
    puts("      Skip this function if the shape file is already in the correct "
         "unit.");
    puts("      Clip and Erase will be done before the unit is changed.");
    puts("      A shift will be done after the unit is changed.");
    puts("  - Shift X and Y coordinates.\n");
    puts("Finally, There can only be one select or unselect in the command "
         "line.");
    puts("         There can only be one clip or erase in the command line.");
    puts("         There can only be one unit and only one shift in the "
         "command line.\n");
    puts("Ex: shputils in.shp out.shp   SELECT countycode 3,5,9,13,17,27");
    puts("    shputils in.shp out.shp   CLIP   10 10 90 90 Touch   FACTOR "
         "Meter Feet");
    puts("    shputils in.shp out.shp   FACTOR Meter 3.0");
    puts("    shputils in.shp out.shp   CLIP   clip.shp Boundary Touch   SHIFT "
         "40 40");
    puts("    shputils in.shp out.shp   SELECT co 112   CLIP clip.shp Boundary "
         "Touch\n");
    puts("USAGE: shputils  <DescribeShape>   {ALL}");
    puts("USAGE: shputils  <InputShape>  <OutShape|AppendShape>");
    puts("   { <FACTOR>       <FEET|MILES|METERS|KM> "
         "<FEET|MILES|METERS|KM|factor> }");
    puts("   { <SHIFT>        <xshift> <yshift> }");
    puts("   { <SELECT|UNSEL> <Item> <valuelist> }");
    puts(
        "   { <CLIP|ERASE>   <xmin> <ymin> <xmax> <ymax> <TOUCH|INSIDE|CUT> }");
    puts(
        "   { <CLIP|ERASE>   <theme>      <BOUNDARY>     <TOUCH|INSIDE|CUT> }");
    puts("     Note: CUT clips lines, polylines, and polygons to the");
    puts("           boundary box (Sutherland-Hodgman algorithm).");

    exit(1);
}

/* -------------------------------------------------------------------- */
/*      Parse command-line arguments into global state.                 */
/*      Calls error()/exit(1) on invalid arguments.                     */
/* -------------------------------------------------------------------- */
void parse_arguments(int argc, char **argv)
{
    if (argc < 2)
        error();
    snprintf(infile, sizeof(infile), "%s", argv[1]);
    if (argc > 2)
    {
        snprintf(outfile, sizeof(outfile), "%s", argv[2]);
        if (strncasecmp2(outfile, "LIST", 0) == 0)
            ilist = true;
        if (strncasecmp2(outfile, "ALL", 0) == 0)
            iall = true;
    }
    if (ilist || iall || argc == 2)
    {
        setext(infile, "shp");
        printf("DESCRIBE: %s\n", infile);
        outfile[0] = '\0';
    }

    for (int i = 3; i < argc; i++)
    {
        if ((strncasecmp2(argv[i], "SEL", 3) == 0) ||
            (strncasecmp2(argv[i], "UNSEL", 5) == 0))
        {
            if (strncasecmp2(argv[i], "UNSEL", 5) == 0)
                iunselect = true;
            i++;
            if (i >= argc)
                error();
            snprintf(selectitem, sizeof(selectitem), "%s", argv[i]);
            i++;
            if (i >= argc)
                error();
            selcount = parse_select_values(argv[i], selectvalues, 150);
            iselect = true;
        }
        else if ((strncasecmp2(argv[i], "CLIP", 4) == 0) ||
                 (strncasecmp2(argv[i], "ERASE", 5) == 0))
        {
            if (strncasecmp2(argv[i], "ERASE", 5) == 0)
                ierase = true;
            i++;
            if (i >= argc)
                error();
            snprintf(clipfile, sizeof(clipfile), "%s", argv[i]);
            sscanf(argv[i], "%lf", &cxmin);
            i++;
            if (i >= argc)
                error();
            if (strncasecmp2(argv[i], "BOUND", 5) == 0)
            {
                setext(clipfile, "shp");
                hSHP = SHPOpen(clipfile, "rb");
                if (hSHP == NULL)
                {
                    printf("ERROR: Unable to open the clip shape file:%s\n",
                           clipfile);
                    exit(1);
                }
                SHPGetInfo(hSHPappend, NULL, NULL, adfBoundsMin, adfBoundsMax);
                cxmin = adfBoundsMin[0];
                cymin = adfBoundsMin[1];
                cxmax = adfBoundsMax[0];
                cymax = adfBoundsMax[1];
                printf("Theme Clip Boundary: (%lf,%lf) - (%lf,%lf)\n", cxmin,
                       cymin, cxmax, cymax);
            }
            else
            {
                sscanf(argv[i], "%lf", &cymin);
                i++;
                if (i >= argc)
                    error();
                sscanf(argv[i], "%lf", &cxmax);
                i++;
                if (i >= argc)
                    error();
                sscanf(argv[i], "%lf", &cymax);
                printf("Clip Box: (%lf,%lf) - (%lf,%lf)\n", cxmin, cymin, cxmax,
                       cymax);
            }
            i++;
            if (i >= argc)
                error();
            if (strncasecmp2(argv[i], "CUT", 3) == 0)
                icut = true;
            else if (strncasecmp2(argv[i], "TOUCH", 5) == 0)
                itouch = true;
            else if (strncasecmp2(argv[i], "INSIDE", 6) == 0)
                iinside = true;
            else
                error();
            iclip = true;
        }
        else if (strncasecmp2(argv[i], "FACTOR", 0) == 0)
        {
            i++;
            if (i >= argc)
                error();
            infactor = findunit(argv[i]);
            if (infactor == 0)
                error();
            iunit = true;
            i++;
            if (i >= argc)
                error();
            outfactor = findunit(argv[i]);
            if (outfactor == 0)
            {
                sscanf(argv[i], "%lf", &factor);
                if (factor == 0)
                    error();
            }
            if (factor == 0)
            {
                if (infactor == 0)
                {
                    puts(
                        "ERROR: Input unit must be defined before output unit");
                    exit(1);
                }
                factor = infactor / outfactor;
            }
            printf("Output file coordinate values will be factored by %lg\n",
                   factor);
        }
        else if (strncasecmp2(argv[i], "SHIFT", 5) == 0)
        {
            i++;
            if (i >= argc)
                error();
            sscanf(argv[i], "%lf", &xshift);
            i++;
            if (i >= argc)
                error();
            sscanf(argv[i], "%lf", &yshift);
            iunit = true;
            printf("X Shift: %lg   Y Shift: %lg\n", xshift, yshift);
        }
        else
        {
            printf("ERROR: Unknown function %s\n", argv[i]);
            error();
        }
    }
}

/* -------------------------------------------------------------------- */
/*      Process all records: select, clip, copy DBF fields,             */
/*      transform coordinates, and write shapes.                        */
/* -------------------------------------------------------------------- */
void process_records(void)
{
    int jRecord = DBFGetRecordCount(hDBFappend);
    const int nFields = DBFGetFieldCount(hDBF);

    for (int iRecord = 0; iRecord < nEntities; iRecord++)
    {
        if (iselect && selectrec(iRecord) == 0)
        {
            psCShape = NULL;
            continue;
        }

        psCShape = SHPReadObject(hSHP, iRecord);
        if (psCShape == NULL)
        {
            fprintf(stderr, "ERROR: Unable to read shape %d\n", iRecord);
            continue;
        }

        if (iclip && clip_boundary() == 0)
        {
            SHPDestroyObject(psCShape);
            psCShape = NULL;
            continue;
        }

        if (!copy_dbf_record(hDBF, iRecord, hDBFappend, jRecord, pt, nFields))
        {
            fprintf(stderr, "Warning: Failed to copy DBF record %d\n", iRecord);
        }
        jRecord++;

        if (iunit)
            transform_coordinates(psCShape, factor, xshift, yshift);

        SHPComputeExtents(psCShape);
        SHPWriteObject(hSHPappend, -1, psCShape);

        SHPDestroyObject(psCShape);
        psCShape = NULL;
    }
}

#ifndef SHPUTILS_NO_MAIN
int main(int argc, char **argv)
{
    parse_arguments(argc, argv);

    openfiles();
    if (DBFGetFieldCount(hDBF) == 0)
    {
        puts("There are no fields in this table!");
        exit(1);
    }

    /* Print out the file bounds. */
    {
        const int iRecord = DBFGetRecordCount(hDBF);
        SHPGetInfo(hSHP, NULL, NULL, adfBoundsMin, adfBoundsMax);
        printf(
            "Input Bounds:  (%lg,%lg) - (%lg,%lg)   Entities: %d   DBF: %d\n",
            adfBoundsMin[0], adfBoundsMin[1], adfBoundsMax[0], adfBoundsMax[1],
            nEntities, iRecord);

        if (strcmp(outfile, "") == 0)
        {
            ti = DBFGetFieldCount(hDBF);
            showitems();
            exit(0);
        }
    }

    if (iclip)
        check_theme_bnd();

    {
        const int jRecord = DBFGetRecordCount(hDBFappend);
        SHPGetInfo(hSHPappend, NULL, NULL, adfBoundsMin, adfBoundsMax);
        if (nEntitiesAppend == 0)
            puts("New Output File\n");
        else
            printf(
                "Append Bounds: (%lg,%lg)-(%lg,%lg)   Entities: %d  DBF: %d\n",
                adfBoundsMin[0], adfBoundsMin[1], adfBoundsMax[0],
                adfBoundsMax[1], nEntitiesAppend, jRecord);
    }

    mergefields();

    if (iselect)
        findselect();

    process_records();

    /* Print out the # of Entities and the file bounds. */
    {
        const int jRecord = DBFGetRecordCount(hDBFappend);
        SHPGetInfo(hSHPappend, &nEntitiesAppend, &nShapeTypeAppend,
                   adfBoundsMin, adfBoundsMax);
        printf(
            "Output Bounds: (%lg,%lg) - (%lg,%lg)   Entities: %d  DBF: %d\n\n",
            adfBoundsMin[0], adfBoundsMin[1], adfBoundsMax[0], adfBoundsMax[1],
            nEntitiesAppend, jRecord);
    }

    SHPClose(hSHP);
    SHPClose(hSHPappend);
    DBFClose(hDBF);
    DBFClose(hDBFappend);

    if (nEntitiesAppend == 0)
    {
        puts("Remove the output files.");
        setext(outfile, "dbf");
        remove(outfile);
        setext(outfile, "shp");
        remove(outfile);
        setext(outfile, "shx");
        remove(outfile);
    }

    return 0;
}
#endif
