/******************************************************************************
 * $Id$
 *
 * Project:  Shapelib
 * Purpose:  Default implementation of file io based on stdio.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2007, Frank Warmerdam
 *
 * This software is available under the following "MIT Style" license,
 * or at the option of the licensee under the LGPL (see LICENSE.LGPL).  This
 * option is discussed in more detail in shapelib.html.
 *
 * --
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
 *
 * $Log$
 * Revision 1.2  2007-12-15 20:25:30  bram
 * dbfopen.c now reads the Code Page information from the DBF file, and exports this information as a string through the DBFGetCodePage function.  This is either the number from the LDID header field ("LDID/<number>") or as the content of an accompanying .CPG file.  When creating a DBF file, the code can be set using DBFCreateEx.
 *
 * Revision 1.1  2007/12/06 06:56:41  fwarmerdam
 * new
 *
 */

#include "shapefil.h"

#include <math.h>
#include <limits.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

SHP_CVSID("$Id$");

/************************************************************************/
/*                              SADFOpen()                              */
/************************************************************************/

SAFile SADFOpen( const char *pszFilename, const char *pszAccess )

{
    return (SAFile) fopen( pszFilename, pszAccess );
}

/************************************************************************/
/*                              SADFRead()                              */
/************************************************************************/

SAOffset SADFRead( void *p, SAOffset size, SAOffset nmemb, SAFile file )

{
    return (SAOffset) fread( p, (size_t) size, (size_t) nmemb, 
                             (FILE *) file );
}

/************************************************************************/
/*                             SADFWrite()                              */
/************************************************************************/

SAOffset SADFWrite( void *p, SAOffset size, SAOffset nmemb, SAFile file )

{
    return (SAOffset) fwrite( p, (size_t) size, (size_t) nmemb, 
                              (FILE *) file );
}

/************************************************************************/
/*                              SADFSeek()                              */
/************************************************************************/

SAOffset SADFSeek( SAFile file, SAOffset offset, int whence )

{
    return (SAOffset) fseek( (FILE *) file, (long) offset, whence );
}

/************************************************************************/
/*                              SADFTell()                              */
/************************************************************************/

SAOffset SADFTell( SAFile file )

{
    return (SAOffset) ftell( (FILE *) file );
}

/************************************************************************/
/*                             SADFFlush()                              */
/************************************************************************/

int SADFFlush( SAFile file )

{
    return fflush( (FILE *) file );
}

/************************************************************************/
/*                             SADFClose()                              */
/************************************************************************/

int SADFClose( SAFile file )

{
    if( file == NULL )
        return;
    return fclose( (FILE *) file );
}

/************************************************************************/
/*                             SADFClose()                              */
/************************************************************************/

int SADRemove( const char *filename )

{
    return remove( filename );
}

/************************************************************************/
/*                              SADError()                              */
/************************************************************************/

void SADError( const char *message )

{
    fprintf( stderr, "%s\n", message );
}

/************************************************************************/
/*                        SASetupDefaultHooks()                         */
/************************************************************************/

void SASetupDefaultHooks( SAHooks *psHooks )

{
    psHooks->FOpen   = SADFOpen;
    psHooks->FRead   = SADFRead;
    psHooks->FWrite  = SADFWrite;
    psHooks->FSeek   = SADFSeek;
    psHooks->FTell   = SADFTell;
    psHooks->FFlush  = SADFFlush;
    psHooks->FClose  = SADFClose;
    psHooks->Remove  = SADRemove;

    psHooks->Error   = SADError;
}
