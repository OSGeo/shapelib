/*
 * Copyright (c) 1999 Carl Anderson
 *
 * This code is in the public domain.
 *
 * This code is based in part on the earlier work of Frank Warmerdam
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shapefil.h"

int main(int argc, char **argv)
{
    /* -------------------------------------------------------------------- */
    /*      Display a usage message.                                        */
    /* -------------------------------------------------------------------- */
    if (argc != 2)
    {
        printf("dbfinfo xbase_file\n");
        exit(1);
    }

    /* -------------------------------------------------------------------- */
    /*      Open the file.                                                  */
    /* -------------------------------------------------------------------- */
    DBFHandle hDBF = DBFOpen(argv[1], "rb");
    if (hDBF == NULL)
    {
        printf("DBFOpen(%s,\"r\") failed.\n", argv[1]);
        exit(2);
    }

    printf("Info for %s\n", argv[1]);

    /* -------------------------------------------------------------------- */
    /*  If there is no data in this file let the user know.                 */
    /* -------------------------------------------------------------------- */
    const int iCount = DBFGetFieldCount(hDBF);
    printf("%d Columns,  %d Records in file\n", iCount,
           DBFGetRecordCount(hDBF));

    /* -------------------------------------------------------------------- */
    /*  Compute offsets to use when printing each of the field              */
    /*  values. We make each field as wide as the field title+1, or         */
    /*  the field value + 1.                                                */
    /* -------------------------------------------------------------------- */
    char ftype[32];
    int nDecimals;
    int nWidth;

    for (int i = 0; i < DBFGetFieldCount(hDBF); i++)
    {
        char szTitle[XBASE_FLDNAME_LEN_READ + 1];

        switch (DBFGetFieldInfo(hDBF, i, szTitle, &nWidth, &nDecimals))
        {
            case FTString:
                strcpy(ftype, "string");
                break;

            case FTInteger:
                strcpy(ftype, "integer");
                break;

            case FTDouble:
                strcpy(ftype, "float");
                break;

            case FTLogical:
                strcpy(ftype, "logical");
                break;

            case FTDate:
                strcpy(ftype, "date");
                break;

            case FTInvalid:
                strcpy(ftype, "invalid/unsupported");
                break;

            default:
                strcpy(ftype, "unknown");
                break;
        }
        printf("%15.15s\t%15s  (%d,%d)\n", szTitle, ftype, nWidth, nDecimals);
    }

    DBFClose(hDBF);

    return (0);
}
