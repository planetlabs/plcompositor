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
/*                             PLCContext()                             */
/************************************************************************/

PLCContext::PLCContext()

{
    outputDS = NULL;
    sourceTraceDS = NULL;
    qualityDS = NULL;
    quiet = FALSE;
    verbose = 0;
    averageBestRatio = 0.0;
    line = -1;
    lastOutputLine = NULL;
    thisOutputLine = NULL;
}

/************************************************************************/
/*                            ~PLCContext()                             */
/************************************************************************/

PLCContext::~PLCContext()

{
    delete lastOutputLine;
    delete thisOutputLine;
}

/************************************************************************/
/*                         getNextOutputLine()                          */
/************************************************************************/

PLCLine *PLCContext::getNextOutputLine()

{
    CPLAssert( outputDS != NULL );

/* -------------------------------------------------------------------- */
/*      Roll over last line buffer.                                     */
/* -------------------------------------------------------------------- */
    if( lastOutputLine != NULL )
        delete lastOutputLine;

    lastOutputLine = thisOutputLine;
    
    line += 1;
    CPLAssert( line >= 0 && line < outputDS->GetRasterYSize() );
    
/* -------------------------------------------------------------------- */
/*      Fetch new line.                                                 */
/* -------------------------------------------------------------------- */
    int  i, width = outputDS->GetRasterXSize();
    thisOutputLine = new PLCLine(width);

    for( i=0; i < outputDS->GetRasterCount(); i++ )
    {
        CPLErr eErr;
        GDALRasterBand *band = outputDS->GetRasterBand(i+1);
        
        if( band->GetColorInterpretation() == GCI_AlphaBand )
            eErr = band->RasterIO(
                GF_Read, 0, line, width, 1, 
                thisOutputLine->getAlpha(), width, 1, GDT_Byte,
                0, 0);
        else
            eErr = band->RasterIO(
                GF_Read, 0, line, width, 1, 
                thisOutputLine->getBand(i), width, 1, GDT_Float32, 
                0, 0);

        if( eErr != CE_None )
            exit(1);
    }

    return thisOutputLine;
}

/************************************************************************/
/*                          writeOutputLine()                           */
/************************************************************************/

void PLCContext::writeOutputLine(bool postProcessing)

{
    CPLAssert( outputDS != NULL );
    
    int  i, width = outputDS->GetRasterXSize();

    for( i=0; i < outputDS->GetRasterCount(); i++ )
    {
        CPLErr eErr;
        GDALRasterBand *band = outputDS->GetRasterBand(i+1);
        
        if( band->GetColorInterpretation() == GCI_AlphaBand )
            eErr = band->RasterIO(
                GF_Write, 0, line, width, 1, 
                thisOutputLine->getAlpha(), width, 1, GDT_Byte, 0, 0);
        else
            eErr = band->RasterIO(
                GF_Write, 0, line, width, 1, 
                thisOutputLine->getBand(i), width, 1, GDT_Float32, 0, 0);

        if( eErr != CE_None )
            exit(1);

        if( sourceTraceDS != NULL && !postProcessing)
        {
            eErr = sourceTraceDS->GetRasterBand(1)->
                RasterIO(GF_Write, 0, line, width, 1, 
                         thisOutputLine->getSource(), width, 1, GDT_UInt16, 
                         0, 0);
        }
        if( qualityDS != NULL && !postProcessing)
        {
            eErr = qualityDS->GetRasterBand(1)->
                RasterIO(GF_Write, 0, line, width, 1, 
                         thisOutputLine->getQuality(), width, 1, GDT_Float32, 
                         0, 0);
        }
    }
}

/************************************************************************/
/*                            isDebugPixel()                            */
/************************************************************************/

void debug_break()
{
    printf( "*" );
    fflush(stdout);
}

int PLCContext::isDebugPixel(int pixel, int line)
{
    for( unsigned int i = 0; i < debugPixels.size(); i += 2 )
    {
        if( debugPixels[i] == pixel && debugPixels[i+1] == line )
        {
            debug_break();
            return TRUE;
        }
    }

    return FALSE;
}

/************************************************************************/
/*                            isDebugLine()                             */
/************************************************************************/
int PLCContext::isDebugLine(int line)
{
    for( unsigned int i = 0; i < debugPixels.size(); i += 2 )
    {
        if( debugPixels[i+1] == line )
            return TRUE;
    }

    return FALSE;
}

/************************************************************************/
/*                      initializeQualityMethods()                      */
/************************************************************************/

void PLCContext::initializeQualityMethods(WJElement compositors)

{
    // Backward compatability mode
    if( compositors == NULL )
    {
        QualityMethodBase *method = NULL;
        CPLDebug("PLC", "initializeQualityMethods() - pre-json mode.");

        if( getStratParam("cloud_quality") != NULL )
        {
            method = QualityMethodBase::CreateQualityFunction(
                this, NULL,
                strategyParams.FetchNameValue("cloud_quality"));
            CPLAssert( method != NULL );
            qualityMethods.push_back(method);
        }

        method = 
            QualityMethodBase::CreateQualityFunction(
                this, NULL, 
                strategyParams.FetchNameValueDef("quality", "darkest"));
        
        CPLAssert( method != NULL );
        qualityMethods.push_back(method);

        if(getStratParam("quality_file") != NULL)
        {
            method = QualityMethodBase::CreateQualityFunction(
                this, NULL, "qualityfromfile");
            CPLAssert( method != NULL );
            qualityMethods.push_back(method);
        }

        if( EQUAL(getStratParam("compositor",""),"median")
            || getStratParam("median_ratio") != NULL
            || getStratParam("quality_percentile") != NULL )
        {
            method = QualityMethodBase::CreateQualityFunction(
                this, NULL, "percentile");
            CPLAssert( method != NULL );
            qualityMethods.push_back(method);
        }

        return;
    }

    WJElement method_def = NULL;
    while( (method_def = _WJEObject(compositors, "[]", WJE_GET, &method_def)) )
    {
        CPLString methodClass = WJEString(method_def, "class", WJE_GET, "");

        if( EQUAL(methodClass,"") )
            CPLError(CE_Fatal, CPLE_AppDefined,
                     "Did not find 'class' field for compositor.");

        QualityMethodBase *method = 
            QualityMethodBase::CreateQualityFunction(
                this, method_def, methodClass);
        if( method == NULL )
            CPLError(CE_Fatal, CPLE_AppDefined,
                     "Compositor class '%s' not recognised.",
                     methodClass.c_str());

        qualityMethods.push_back(method);
    }
}

/************************************************************************/
/*                         initializeFromJson()                         */
/*                                                                      */
/*      Initialize the context from the JSON definition document.       */
/*      Some options may already have been (or still to be) set from    */
/*      commandline options as well as using the JSON.                  */
/************************************************************************/

void PLCContext::initializeFromJson(WJElement doc)

{
    outputFilename = WJEString(doc, "output_file", WJE_GET, outputFilename);
    sourceTraceFilename = 
        WJEString(doc, "source_trace", WJE_GET, sourceTraceFilename);
    qualityFilename = 
        WJEString(doc, "quality_output", WJE_GET, qualityFilename);
    averageBestRatio = 
        WJEDouble(doc, "average_best_ratio", WJE_GET, 0.0);
    sourceSieveThreshold = (int)
        WJEInt32(doc, "source_sieve_threshold", WJE_GET, 0);

    initializeQualityMethods( WJEArray(doc, "compositors", WJE_GET) );
    
    WJElement input_def = NULL;
    while( (input_def = _WJEObject(doc, "inputs[]", WJE_GET, &input_def)) )
    {
        // Consume a whole PLCInput definition.
        PLCInput *input = new PLCInput(inputFiles.size());
        input->ConsumeJson(input_def);
        inputFiles.push_back(input);
    }
}
