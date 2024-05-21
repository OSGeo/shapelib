/*
 * Copyright (c) 1995 Frank Warmerdam
 *
 * This code is in the public domain.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shapefil.h"

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("dbfcat [-v] [-f] from_DBFfile to_DBFfile\n");
        exit(1);
    }

    int force = 0;
    int verbose = 0;
    int shift = 0;
    if (strcmp("-v", argv[1]) == 0)
    {
        shift = 1;
        verbose = 1;
    }
    if (strcmp("-f", argv[1 + shift]) == 0)
    {
        shift++;
        force = 1;
    }
    if (strcmp("-v", argv[1 + shift]) == 0)
    {
        shift++;
        verbose = 1;
    }

    DBFHandle hDBF = DBFOpen(argv[1 + shift], "rb");
    if (hDBF == NULL)
    {
        printf("DBFOpen(%s,\"r\") failed for From_DBF.\n", argv[1 + shift]);
        exit(2);
    }

    DBFHandle cDBF = DBFOpen(argv[2 + shift], "rb+");
    if (cDBF == NULL)
    {
        printf("DBFOpen(%s,\"rb+\") failed for To_DBF.\n", argv[2 + shift]);
        DBFClose(hDBF);
        exit(2);
    }

    const int hflds = DBFGetFieldCount(hDBF);
    if (hflds == 0)
    {
        printf("There are no fields in this table!\n");
        DBFClose(hDBF);
        DBFClose(cDBF);
        exit(3);
    }

    const int cflds = DBFGetFieldCount(cDBF);

    int matches = 0;
    int mismatch = 0;

    int nWidth;
    int nDecimals;
    char fld_m[256];
    int cnWidth;
    int cnDecimals;
    const char type_names[6][15] = {"string",  "integer", "double",
                                    "logical", "date",    "invalid"};

    for (int i = 0; i < hflds; i++)
    {
        char szTitle[XBASE_FLDNAME_LEN_READ + 1];
        const DBFFieldType hType =
            DBFGetFieldInfo(hDBF, i, szTitle, &nWidth, &nDecimals);

        char cname[XBASE_FLDNAME_LEN_READ + 1];
        fld_m[i] = -1;
        for (int j = 0; j < cflds; j++)
        {
            const DBFFieldType cType =
                DBFGetFieldInfo(cDBF, j, cname, &cnWidth, &cnDecimals);
            if (strcmp(cname, szTitle) == 0)
            {
                if (hType != cType)
                {
                    fprintf(stderr, "Incompatible fields %s(%s) != %s(%s),\n",
                            type_names[hType], szTitle, type_names[cType],
                            cname);
                    mismatch = 1;
                }
                fld_m[i] = j;
                if (verbose)
                {
                    printf("%s  %s(%d,%d) <- %s  %s(%d,%d)\n", cname,
                           type_names[cType], cnWidth, cnDecimals, szTitle,
                           type_names[hType], nWidth, nDecimals);
                }
                j = cflds;
                matches = 1;
            }
        }
    }

    if ((matches == 0) && !force)
    {
        fprintf(
            stderr,
            "ERROR: No field names match for tables, cannot proceed\n   use "
            "-f to force processing using blank records\n");
        DBFClose(hDBF);
        DBFClose(cDBF);
        exit(-1);
    }
    if (mismatch && !force)
    {
        fprintf(
            stderr,
            "ERROR: field type mismatch cannot proceed\n    use -f to force "
            "processing using attempted conversions\n");
        DBFClose(hDBF);
        DBFClose(cDBF);
        exit(-1);
    }

    const int nRecords = DBFGetRecordCount(cDBF);
    for (int iRecord = 0; iRecord < DBFGetRecordCount(hDBF); iRecord++)
    {
        if (DBFIsRecordDeleted(hDBF, iRecord))
        {
            continue;
        }
        const int ciRecord = DBFGetRecordCount(cDBF);
        for (int i = 0; i < hflds; i++)
        {
            const int ci = fld_m[i];
            if (ci != -1)
            {
                char cTitle[XBASE_FLDNAME_LEN_READ + 1];
                const DBFFieldType cType =
                    DBFGetFieldInfo(cDBF, ci, cTitle, &cnWidth, &cnDecimals);

                switch (cType)
                {
                    case FTString:
                        DBFWriteStringAttribute(
                            cDBF, ciRecord, ci,
                            DBFReadStringAttribute(hDBF, iRecord, i));
                        break;

                    case FTLogical:
                    case FTDate:
                        DBFWriteAttributeDirectly(
                            cDBF, ciRecord, ci,
                            DBFReadStringAttribute(hDBF, iRecord, i));
                        break;

                    case FTInteger:
                        DBFWriteIntegerAttribute(
                            cDBF, ciRecord, ci,
                            DBFReadIntegerAttribute(hDBF, iRecord, i));
                        break;

                    case FTDouble:
                        DBFWriteDoubleAttribute(
                            cDBF, ciRecord, ci,
                            DBFReadDoubleAttribute(hDBF, iRecord, i));
                        break;

                    case FTInvalid:
                        break;
                }
            }
        } /* fields names match */
    }

    if (verbose)
    {
        const int ncRecords = DBFGetRecordCount(cDBF) - nRecords;
        printf(" %d record%s appended\n\n", ncRecords,
               ncRecords == 1 ? "" : "s");
    }
    DBFClose(hDBF);
    DBFClose(cDBF);

    return (0);
}
