/******************************************************************************
 *
 * Project:  DXF Translator
 * Purpose:  Implements OGRDXFBlocksLayer class.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2010, Frank Warmerdam <warmerdam@pobox.com>
 *
 * SPDX-License-Identifier: MIT
 ****************************************************************************/

#include "ogr_dxf.h"
#include "cpl_conv.h"

/************************************************************************/
/*                         OGRDXFBlocksLayer()                          */
/************************************************************************/

OGRDXFBlocksLayer::OGRDXFBlocksLayer(OGRDXFDataSource *poDSIn)
    : poDS(poDSIn), poFeatureDefn(new OGRFeatureDefn("blocks")), iNextFID(0)
{
    OGRDXFBlocksLayer::ResetReading();

    poFeatureDefn->Reference();

    int nModes = ODFM_None;
    if (!poDS->InlineBlocks())
        nModes |= ODFM_IncludeBlockFields;
    if (poDS->ShouldIncludeRawCodeValues())
        nModes |= ODFM_IncludeRawCodeValues;
    if (poDS->In3DExtensibleMode())
        nModes |= ODFM_Include3DModeFields;
    OGRDXFDataSource::AddStandardFields(poFeatureDefn, nModes);
}

/************************************************************************/
/*                         ~OGRDXFBlocksLayer()                         */
/************************************************************************/

OGRDXFBlocksLayer::~OGRDXFBlocksLayer()

{
    if (m_nFeaturesRead > 0 && poFeatureDefn != nullptr)
    {
        CPLDebug("DXF", "%d features read on layer '%s'.", (int)m_nFeaturesRead,
                 poFeatureDefn->GetName());
    }

    if (poFeatureDefn)
        poFeatureDefn->Release();

    while (!apoPendingFeatures.empty())
    {
        delete apoPendingFeatures.front();
        apoPendingFeatures.pop();
    }
}

/************************************************************************/
/*                            ResetReading()                            */
/************************************************************************/

void OGRDXFBlocksLayer::ResetReading()

{
    iNextFID = 0;
    while (!apoPendingFeatures.empty())
    {
        OGRDXFFeature *poFeature = apoPendingFeatures.front();
        apoPendingFeatures.pop();
        delete poFeature;
    }
    oIt = poDS->GetBlockMap().begin();
}

/************************************************************************/
/*                      GetNextUnfilteredFeature()                      */
/************************************************************************/

OGRDXFFeature *OGRDXFBlocksLayer::GetNextUnfilteredFeature()

{
    OGRDXFFeature *poFeature = nullptr;

    /* -------------------------------------------------------------------- */
    /*      If we have pending features, return one of them.                */
    /* -------------------------------------------------------------------- */
    if (!apoPendingFeatures.empty())
    {
        poFeature = apoPendingFeatures.front();
        apoPendingFeatures.pop();

        poFeature->SetFID(iNextFID++);
        poFeature->SetField("Block", osBlockName.c_str());
        if (poFeature->GetAttributeTag() != "")
        {
            poFeature->SetField("AttributeTag", poFeature->GetAttributeTag());
        }

        m_nFeaturesRead++;
        return poFeature;
    }

    /* -------------------------------------------------------------------- */
    /*      Are we out of features?                                         */
    /* -------------------------------------------------------------------- */
    while (oIt != poDS->GetBlockMap().end())
    {
        poFeature = new OGRDXFFeature(poFeatureDefn);

        // Let's insert this block at the origin with no rotation and scale.
        OGRDXFLayer oTempLayer(poDS);
        poFeature = oTempLayer.InsertBlockInline(
            CPLGetErrorCounter(), oIt->first, OGRDXFInsertTransformer(),
            poFeature, apoPendingFeatures, false,
            poDS->ShouldMergeBlockGeometries());

        osBlockName = oIt->first;
        ++oIt;

        if (!poFeature)
        {
            if (apoPendingFeatures.empty())
            {
                // This block must have been empty. Move onto the next block
                continue;
            }
            else
            {
                poFeature = apoPendingFeatures.front();
                apoPendingFeatures.pop();
            }
        }

        poFeature->SetFID(iNextFID++);
        poFeature->SetField("Block", osBlockName.c_str());
        if (poFeature->GetAttributeTag() != "")
        {
            poFeature->SetField("AttributeTag", poFeature->GetAttributeTag());
        }

        m_nFeaturesRead++;
        return poFeature;
    }

    // No more blocks left.
    return nullptr;
}

/************************************************************************/
/*                           GetNextFeature()                           */
/************************************************************************/

OGRFeature *OGRDXFBlocksLayer::GetNextFeature()

{
    while (true)
    {
        OGRFeature *poFeature = GetNextUnfilteredFeature();

        if (poFeature == nullptr)
            return nullptr;

        if ((m_poFilterGeom == nullptr ||
             FilterGeometry(poFeature->GetGeometryRef())) &&
            (m_poAttrQuery == nullptr || m_poAttrQuery->Evaluate(poFeature)))
        {
            return poFeature;
        }

        delete poFeature;
    }
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRDXFBlocksLayer::TestCapability(const char *pszCap)

{
    return EQUAL(pszCap, OLCStringsAsUTF8);
}
