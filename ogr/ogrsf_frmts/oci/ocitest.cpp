/******************************************************************************
 *
 * Project:  Oracle Spatial Driver
 * Purpose:  Test mainline for Oracle Spatial Driver low level functions.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2002, Frank Warmerdam <warmerdam@pobox.com>
 *
 * SPDX-License-Identifier: MIT
 ****************************************************************************/

#include "ogr_oci.h"
#include "cpl_conv.h"

/************************************************************************/
/*                                main()                                */
/************************************************************************/

int main(int nArgc, char **papszArgv)

{
    OGROCISession *poSession = nullptr;
    const char *pszStatement = "SELECT * FROM NEPSITE";
    char **papszResult;

    if (nArgc > 1)
        pszStatement = papszArgv[1];

    poSession = OGRGetOCISession("system", "LetoKing", "");
    if (poSession == nullptr)
        exit(1);

    OGROCIStatement oStatement(poSession);

    if (oStatement.Execute(pszStatement) == CE_Failure)
        exit(2);

    while ((papszResult = oStatement.SimpleFetchRow()) != nullptr)
    {
        OGRFeatureDefn *poDefn = oStatement.GetResultDefn();
        int nColCount = poDefn->GetFieldCount();
        int i;

        printf("\n");
        for (i = 0; i < nColCount; i++)
        {
            printf("  %s = %s\n", poDefn->GetFieldDefn(i)->GetNameRef(),
                   papszResult[i]);
        }
    }
}
