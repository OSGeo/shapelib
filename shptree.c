/******************************************************************************
 * $Id$
 *
 * Project:  Shapelib
 * Purpose:  Implementation of quadtree building and searching functions.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
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
 * Revision 1.1  1999-05-18 17:49:20  warmerda
 * New
 *
 */

static char rcsid[] = 
  "$Id$";

#include "shapefil.h"

#include <math.h>
#include <assert.h>

#ifndef TRUE
#  define TRUE 1
#  define FALSE 0
#endif


/* -------------------------------------------------------------------- */
/*      If the following is 0.5, nodes will be split in half.  If it    */
/*      is 0.6 then each subnode will contain 60% of the parent         */
/*      node, with 20% representing overlap.  This can be help to       */
/*      prevent small objects on a boundary from shifting too high      */
/*      up the tree.                                                    */
/* -------------------------------------------------------------------- */

#define SHP_SPLIT_RATIO	0.55

/************************************************************************/
/*                             SfRealloc()                              */
/*                                                                      */
/*      A realloc cover function that will access a NULL pointer as     */
/*      a valid input.                                                  */
/************************************************************************/

static void * SfRealloc( void * pMem, int nNewSize )

{
    if( pMem == NULL )
        return( (void *) malloc(nNewSize) );
    else
        return( (void *) realloc(pMem,nNewSize) );
}

/************************************************************************/
/*                          SHPTreeNodeInit()                           */
/*                                                                      */
/*      Initialize a tree node.                                         */
/************************************************************************/

static SHPTreeNode *SHPTreeNodeCreate( double * padfBoundsMin,
                                       double * padfBoundsMax )

{
    SHPTreeNode	*psTreeNode;

    psTreeNode = (SHPTreeNode *) malloc(sizeof(SHPTreeNode));

    psTreeNode->nShapeCount = 0;
    psTreeNode->panShapeIds = NULL;
    psTreeNode->papsShapeObj = NULL;
    
    psTreeNode->psSubNode1 = NULL;
    psTreeNode->psSubNode2 = NULL;

    if( padfBoundsMin != NULL )
        memcpy( psTreeNode->adfBoundsMin, padfBoundsMin, sizeof(double) * 4 );

    if( padfBoundsMax != NULL )
        memcpy( psTreeNode->adfBoundsMax, padfBoundsMax, sizeof(double) * 4 );

    return psTreeNode;
}


/************************************************************************/
/*                           SHPCreateTree()                            */
/************************************************************************/

SHPTree *SHPCreateTree( SHPHandle hSHP, int nDimension, int nMaxDepth,
                        double *padfBoundsMin, double *padfBoundsMax )

{
    SHPTree	*psTree;

    if( padfBoundsMin == NULL && hSHP == NULL )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Allocate the tree object                                        */
/* -------------------------------------------------------------------- */
    psTree = (SHPTree *) malloc(sizeof(SHPTree));

    psTree->hSHP = hSHP;
    psTree->nMaxDepth = nMaxDepth;
    psTree->nDimension = nDimension;

/* -------------------------------------------------------------------- */
/*      If no max depth was defined, try to select a reasonable one     */
/*      that implies approximately 8 shapes per node.                   */
/* -------------------------------------------------------------------- */
    if( psTree->nMaxDepth == 0 && hSHP != NULL )
    {
        int	nMaxNodeCount = 1;
        int	nShapeCount;

        SHPGetInfo( hSHP, &nShapeCount, NULL, NULL, NULL );
        while( nMaxNodeCount*4 < nShapeCount )
        {
            psTree->nMaxDepth += 1;
            nMaxNodeCount = nMaxNodeCount * 2;
        }
    }

/* -------------------------------------------------------------------- */
/*      Allocate the root node.                                         */
/* -------------------------------------------------------------------- */
    psTree->psRoot = SHPTreeNodeCreate( padfBoundsMin, padfBoundsMax );

/* -------------------------------------------------------------------- */
/*      Assign the bounds to the root node.  If none are passed in,     */
/*      use the bounds of the provided file otherwise the create        */
/*      function will have already set the bounds.                      */
/* -------------------------------------------------------------------- */
    if( padfBoundsMin == NULL )
    {
        SHPGetInfo( hSHP, NULL, NULL,
                    psTree->psRoot->adfBoundsMin, 
                    psTree->psRoot->adfBoundsMax );
    }

/* -------------------------------------------------------------------- */
/*      If we have a file, insert all it's shapes into the tree.        */
/* -------------------------------------------------------------------- */
    if( hSHP != NULL )
    {
        int	iShape, nShapeCount;
        
        SHPGetInfo( hSHP, &nShapeCount, NULL, NULL, NULL );

        for( iShape = 0; iShape < nShapeCount; iShape++ )
        {
            SHPObject	*psShape;
            
            psShape = SHPReadObject( hSHP, iShape );
            SHPTreeAddShapeId( psTree, psShape );
            SHPDestroyObject( psShape );
        }
    }        

    return psTree;
}

/************************************************************************/
/*                         SHPDestroyTreeNode()                         */
/************************************************************************/

static void SHPDestroyTreeNode( SHPTreeNode * psTreeNode )

{
    if( psTreeNode->psSubNode1 != NULL )
        SHPDestroyTreeNode( psTreeNode->psSubNode1 );

    if( psTreeNode->psSubNode2 != NULL )
        SHPDestroyTreeNode( psTreeNode->psSubNode2 );

    if( psTreeNode->panShapeIds != NULL )
        free( psTreeNode->panShapeIds );

    if( psTreeNode->papsShapeObj != NULL )
    {
        int		i;

        for( i = 0; i < psTreeNode->nShapeCount; i++ )
        {
            if( psTreeNode->papsShapeObj[i] != NULL )
                SHPDestroyObject( psTreeNode->papsShapeObj[i] );
        }

        free( psTreeNode->papsShapeObj );
    }

    free( psTreeNode );
}

/************************************************************************/
/*                           SHPDestroyTree()                           */
/************************************************************************/

void SHPDestroyTree( SHPTree * psTree )

{
    SHPDestroyTreeNode( psTree->psRoot );
    free( psTree );
}

/************************************************************************/
/*                           SHPCheckBounds()                           */
/*                                                                      */
/*      Does the given shape fit within the indicated extents?          */
/************************************************************************/

static int SHPCheckBounds( SHPObject * psObject, int nDimension,
                           double * padfBoundsMin, double * padfBoundsMax )

{
    if( psObject->dfXMin < padfBoundsMin[0]
        || psObject->dfXMax > padfBoundsMax[0] )
        return FALSE;
    
    if( psObject->dfYMin < padfBoundsMin[1]
        || psObject->dfYMax > padfBoundsMax[1] )
        return FALSE;

    if( nDimension == 2 )
        return TRUE;
    
    if( psObject->dfZMin < padfBoundsMin[2]
        || psObject->dfZMax < padfBoundsMax[2] )
        return FALSE;
        
    if( nDimension == 3 )
        return TRUE;

    if( psObject->dfMMin < padfBoundsMin[3]
        || psObject->dfMMax < padfBoundsMax[3] )
        return FALSE;

    return TRUE;
}

/************************************************************************/
/*                         SHPTreeSplitBounds()                         */
/*                                                                      */
/*      Split a region into two subregion evenly, cutting along the     */
/*      longest dimension.                                              */
/************************************************************************/

void SHPTreeSplitBounds( double *padfBoundsMinIn, double *padfBoundsMaxIn,
                         double *padfBoundsMin1, double * padfBoundsMax1,
                         double *padfBoundsMin2, double * padfBoundsMax2 )

{
/* -------------------------------------------------------------------- */
/*      The output bounds will be very similar to the input bounds,     */
/*      so just copy over to start.                                     */
/* -------------------------------------------------------------------- */
    memcpy( padfBoundsMin1, padfBoundsMinIn, sizeof(double) * 4 );
    memcpy( padfBoundsMax1, padfBoundsMaxIn, sizeof(double) * 4 );
    memcpy( padfBoundsMin2, padfBoundsMinIn, sizeof(double) * 4 );
    memcpy( padfBoundsMax2, padfBoundsMaxIn, sizeof(double) * 4 );
    
/* -------------------------------------------------------------------- */
/*      Split in X direction.                                           */
/* -------------------------------------------------------------------- */
    if( (padfBoundsMaxIn[0] - padfBoundsMinIn[0])
        			> (padfBoundsMaxIn[1] - padfBoundsMinIn[1]) )
    {
        double	dfRange = padfBoundsMaxIn[0] - padfBoundsMinIn[0];

        padfBoundsMax1[0] = padfBoundsMinIn[0] + dfRange * SHP_SPLIT_RATIO;
        padfBoundsMin2[0] = padfBoundsMaxIn[0] - dfRange * SHP_SPLIT_RATIO;
    }

/* -------------------------------------------------------------------- */
/*      Otherwise split in Y direction.                                 */
/* -------------------------------------------------------------------- */
    else
    {
        double	dfRange = padfBoundsMaxIn[1] - padfBoundsMinIn[1];

        padfBoundsMax1[1] = padfBoundsMinIn[1] + dfRange * SHP_SPLIT_RATIO;
        padfBoundsMin2[1] = padfBoundsMaxIn[1] - dfRange * SHP_SPLIT_RATIO;
    }
}

/************************************************************************/
/*                       SHPTreeNodeAddShapeId()                        */
/************************************************************************/

static int
SHPTreeNodeAddShapeId( SHPTreeNode * psTreeNode, SHPObject * psObject,
                       int nMaxDepth, int nDimension )

{
/* -------------------------------------------------------------------- */
/*      If there are subnodes, then consider wiether this object        */
/*      will fit in them.                                               */
/* -------------------------------------------------------------------- */
    if( nMaxDepth > 1 && psTreeNode->psSubNode1 != NULL )
    {
        if( SHPCheckBounds(psObject, nDimension,
                           psTreeNode->psSubNode1->adfBoundsMin,
                           psTreeNode->psSubNode1->adfBoundsMax))
        {
            return SHPTreeNodeAddShapeId( psTreeNode->psSubNode1, psObject,
                                          nMaxDepth-1, nDimension );
        }

        if( SHPCheckBounds(psObject, nDimension,
                           psTreeNode->psSubNode2->adfBoundsMin,
                           psTreeNode->psSubNode2->adfBoundsMax))
        {
            return SHPTreeNodeAddShapeId( psTreeNode->psSubNode2, psObject,
                                          nMaxDepth-1, nDimension );
        }
    }

/* -------------------------------------------------------------------- */
/*      Otherwise, consider creating subnodes if could fit into         */
/*      them, and adding to the appropriate subnode.                    */
/* -------------------------------------------------------------------- */
    else if( nMaxDepth > 1 )
    {
        double	adfBoundsMin1[4], adfBoundsMax1[4];
        double	adfBoundsMin2[4], adfBoundsMax2[4];

        SHPTreeSplitBounds( psTreeNode->adfBoundsMin, psTreeNode->adfBoundsMax,
                            adfBoundsMin1, adfBoundsMax1,
                            adfBoundsMin2, adfBoundsMax2 );

        if( SHPCheckBounds(psObject, nDimension, adfBoundsMin1, adfBoundsMax1))
        {
            psTreeNode->psSubNode1 = SHPTreeNodeCreate( adfBoundsMin1,
                                                        adfBoundsMax1 );
            psTreeNode->psSubNode2 = SHPTreeNodeCreate( adfBoundsMin2,
                                                        adfBoundsMax2 );

            return( SHPTreeNodeAddShapeId( psTreeNode->psSubNode1, psObject,
                                           nMaxDepth - 1, nDimension ) );
        }
        else if( SHPCheckBounds(psObject, nDimension,
                                adfBoundsMin2, adfBoundsMax2) )
        {
            psTreeNode->psSubNode1 = SHPTreeNodeCreate( adfBoundsMin1,
                                                        adfBoundsMax1 );
            psTreeNode->psSubNode2 = SHPTreeNodeCreate( adfBoundsMin2,
                                                        adfBoundsMax2 );

            return( SHPTreeNodeAddShapeId( psTreeNode->psSubNode2, psObject,
                                           nMaxDepth - 1, nDimension ) );
        }
    }

/* -------------------------------------------------------------------- */
/*      If none of that worked, just add it to this nodes list.         */
/* -------------------------------------------------------------------- */
    psTreeNode->nShapeCount++;

    psTreeNode->panShapeIds =
        SfRealloc( psTreeNode->panShapeIds,
                   sizeof(int) * psTreeNode->nShapeCount );
    psTreeNode->panShapeIds[psTreeNode->nShapeCount-1] = psObject->nShapeId;

    if( psTreeNode->papsShapeObj != NULL )
    {
        psTreeNode->papsShapeObj =
            SfRealloc( psTreeNode->papsShapeObj,
                       sizeof(void *) * psTreeNode->nShapeCount );
        psTreeNode->papsShapeObj[psTreeNode->nShapeCount-1] = NULL;
    }

    return TRUE;
}

/************************************************************************/
/*                         SHPTreeAddShapeId()                          */
/*                                                                      */
/*      Add a shape to the tree, but don't keep a pointer to the        */
/*      object data, just keep the shapeid.                             */
/************************************************************************/

int SHPTreeAddShapeId( SHPTree * psTree, SHPObject * psObject )

{
    return( SHPTreeNodeAddShapeId( psTree->psRoot, psObject,
                                   psTree->nMaxDepth, psTree->nDimension ) );
}
