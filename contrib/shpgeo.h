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
 * shpgeo.h
 *
 * support for geometric and other additions to shapelib
 */

/* I'm using some shorthand throughout this file
 *      R+ is a Clockwise Ring and is the positive portion of an object
 *      R- is a CounterClockwise Ring and is a hole in a R+
 *      A complex object is one having at least one R-
 *      A compound object is one having more than one R+
 *	A simple object has one and only one element (R+ or R-)
 *
 *	The closed ring constraint is for polygons and assumed here
 *	Arcs or LineStrings I am calling Rings (generically open or closed)
 *	Point types are vertices or lists of vertices but not Rings
 *
 *   SHPT_POLYGON, SHPT_POLYGONZ, SHPT_POLYGONM and SHPT_MULTIPATCH
 *   can have SHPObjects that are compound as well as complex
 *
 *   SHP_POINT and its Z and M derivatives are strictly simple
 *   MULTI_POINT, SHPT_ARC and their derivatives may be simple or compound
 *
 */

#ifndef SHPGEO_H
#define SHPGEO_H

#ifdef __cplusplus
extern "C"
{
#endif

#define SHPD_POINT 1
#define SHPD_LINE 2
#define SHPD_AREA 4
#define SHPD_Z 8
#define SHPD_MEASURE 16

/* move these into a ogis header file ogis.h */
#define OGIST_UNKNOWN 0
#define OGIST_POINT 1
#define OGIST_LINESTRING 2
#define OGIST_POLYGON 3
#define OGIST_MULTIPOINT 4
#define OGIST_MULTILINE 5
#define OGIST_MULTIPOLYGON 6
#define OGIST_GEOMCOLL 7

    typedef struct
    {
        int StreamPos;
        int NeedSwap;
        char *wStream;
    } WKBStreamObj;

    typedef struct
    {
        double x;
        double y;
    } PT;

    typedef struct
    {
        int cParts;
        SHPObject *SHPObj;
    } SHPObjectList;

#define LSB_ORDER (int)1

    extern char *asFileName(const char *fil, const char *ext);

    extern int SHPDimension(int SHPType);

    extern double SHPArea_2d(const SHPObject *psCShape);
    extern int SHPRingDir_2d(const SHPObject *psCShape, int Ring);
    extern double SHPLength_2d(const SHPObject *psCShape);
    extern PT SHPCentrd_2d(const SHPObject *psCShape);
    extern PT SHPPointinPoly_2d(const SHPObject *psCShape);
    extern PT *SHPPointsinPoly_2d(const SHPObject *psCShape);

    extern int RingCentroid_2d(int nVertices, const double *a, const double *b,
                               PT *C, double *Area);
    extern double RingLength_2d(int nVertices, const double *a,
                                const double *b);
    extern double RingArea_2d(int nVertices, const double *a, const double *b);

    extern SHPObject *SHPClone(const SHPObject *psCShape, int lowPart,
                               int highPart);
    extern SHPObject *SHPUnCompound(const SHPObject *psCShape, int *ringNumber);
    extern SHPObject *SHPIntersect_2d(const SHPObject *a, const SHPObject *b);

    extern int SHPWriteOGisWKB(WKBStreamObj *stream_obj,
                               const SHPObject *psCShape);
    extern SHPObject *SHPReadOGisWKB(WKBStreamObj *stream_obj);

    int SHPWriteOGisPolygon(WKBStreamObj *stream_obj,
                            const SHPObject *psCShape);
    int SHPWriteOGisLine(WKBStreamObj *stream_obj, const SHPObject *psCShape);
    int SHPWriteOGisPoint(WKBStreamObj *stream_obj, const SHPObject *psCShape);

    SHPObject *SHPReadOGisPolygon(WKBStreamObj *stream_obj);
    SHPObject *SHPReadOGisLine(WKBStreamObj *stream_obj);
    SHPObject *SHPReadOGisPoint(WKBStreamObj *stream_obj);

    extern int SHPClean(SHPObject *psCShape);
    extern int SHPOGisType(int GeomType, int toOGis);

    void swapD(void *so, const unsigned char *in, long bytes);
    void swapW(void *so, const unsigned char *in, long bytes);
    void SwapG(void *so, const void *in, int this_cnt, int this_size);

#ifdef __cplusplus
}
#endif

#endif /* ndef SHPGEO_H	*/
