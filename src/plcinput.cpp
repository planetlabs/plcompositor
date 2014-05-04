/**
 * Copyright 2014, Planet Labs, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "compositor.h"

/************************************************************************/
/*                              PLCInput()                              */
/************************************************************************/

PLCInput::PLCInput()
{
    DS = NULL;
    cloudDS = NULL;
    qualityMethod = NULL;
    cloudQualityMethod = NULL;
}

/************************************************************************/
/*                             ~PLCInput()                              */
/************************************************************************/

PLCInput::~PLCInput()
{
}

/************************************************************************/
/*                            ConsumeArgs()                             */
/*                                                                      */
/*      Parse arguments defining an input file and related              */
/*      information.                                                    */
/*                                                                      */
/*      Like:   [-i input_file [-c cloudmask] [-qm name value]*]*       */
/************************************************************************/
int PLCInput::ConsumeArgs(int argc, char **argv)
{
    if( !EQUAL(argv[0],"-i") || argc < 2)
        CPLError(CE_Fatal, CPLE_AppDefined,
                 "Did not get -i <filename> for input group.");

    filename = argv[1];

    int iArg = 2;

    while(iArg < argc) 
    {
        if( iArg < argc-1 && EQUAL(argv[iArg],"-c") )
        {
            cloudMask = argv[iArg+1];
            iArg += 2;
        }
        else if( iArg < argc-2 && EQUAL(argv[iArg],"-qm") )
        {
            qualityMetrics[argv[iArg+1]] = CPLAtof(argv[iArg+2]);
            iArg += 3;
        }
        else
            break;
    }

    return iArg;
}

/************************************************************************/
/*                             Initialize()                             */
/*                                                                      */
/*      Ensure all arguments make sense, initialize quality methods,    */
/*      open file(s).                                                   */
/************************************************************************/

void PLCInput::Initialize(PLCContext *plContext)

{
    getDS();
    getCloudDS();

    qualityMethod = QualityMethodBase::CreateQualityFunction(
        plContext, 
        this,
        plContext->strategyParams.FetchNameValueDef("quality", "darkest"));
    CPLAssert( qualityMethod );
 
    if( plContext->strategyParams.FetchNameValue("cloud_quality") )
        cloudQualityMethod = QualityMethodBase::CreateQualityFunction(
            plContext, 
            this,
            plContext->strategyParams.FetchNameValue("cloud_quality"));
}

/************************************************************************/
/*                               getDS()                                */
/************************************************************************/

GDALDataset *PLCInput::getDS()

{
    if( DS == NULL )
    {
        DS = (GDALDataset *) GDALOpen(filename, GA_ReadOnly);
        if( DS == NULL )
            exit(1);
    }

    return DS;
}

/************************************************************************/
/*                             getCloudDS()                             */
/************************************************************************/

GDALDataset *PLCInput::getCloudDS()

{
    if( cloudDS == NULL && !EQUAL(cloudMask,""))
    {
        cloudDS = (GDALDataset *) GDALOpen(cloudMask, GA_ReadOnly);
        if( cloudDS == NULL )
            exit(1);
    }

    return cloudDS;
}

/************************************************************************/
/*                              getLine()                               */
/************************************************************************/

PLCLine *PLCInput::getLine(int line)

{
    getDS();
    
    int  i, width = DS->GetRasterXSize();
    PLCLine *lineObj = new PLCLine(width);

/* -------------------------------------------------------------------- */
/*      Load imagery.                                                   */
/* -------------------------------------------------------------------- */
    for( i=0; i < DS->GetRasterCount(); i++ )
    {
        CPLErr eErr;
        GDALRasterBand *band = DS->GetRasterBand(i+1);
        
        if( band->GetColorInterpretation() == GCI_AlphaBand )
            eErr = band->RasterIO(GF_Read, 0, line, width, 1, 
                                  lineObj->getAlpha(), width, 1, GDT_Byte,
                                  0, 0);
        else
            eErr = band->RasterIO(GF_Read, 0, line, width, 1, 
                                  lineObj->getBand(i), width, 1, GDT_Int16, 
                                  0, 0);

        if( eErr != CE_None )
            exit(1);
    }

/* -------------------------------------------------------------------- */
/*      Load cloud mask                                                 */
/* -------------------------------------------------------------------- */
    if( !EQUAL(cloudMask,"") )
    {
        getCloudDS();
        
        CPLErr eErr;
        GDALRasterBand *band = cloudDS->GetRasterBand(1);

        eErr = band->RasterIO(GF_Read, 0, line, width, 1, 
                              lineObj->getCloud(), width, 1, GDT_UInt16, 
                              0, 0);

        if( eErr != CE_None )
            exit(1);
    }

    return lineObj;
}

/************************************************************************/
/*                               getQM()                                */
/************************************************************************/

double PLCInput::getQM(const char *key, double defaultValue)

{
    if(qualityMetrics.count(key) > 0)
        return qualityMetrics[key];
    else
        return defaultValue;
}

/************************************************************************/
/*                           computeQuality()                           */
/*                                                                      */
/*      Compute the line quality, and possibly the cloud quality in     */
/*      which case the cloud quality is merged back into the core       */
/*      quality.                                                        */
/************************************************************************/

int PLCInput::computeQuality(PLCContext *context, PLCLine *line)

{
    CPLAssert( qualityMethod != NULL );
    
    if( !qualityMethod->computeQuality(line) )
        return FALSE;

    if( cloudQualityMethod != NULL )
    {
        if( !cloudQualityMethod->computeQuality(line) )
            return FALSE;
        line->mergeCloudQuality();
    }

    return TRUE;
}
