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
    quiet = FALSE;
    verbose = 0;
}

/************************************************************************/
/*                            ~PLCContext()                             */
/************************************************************************/

PLCContext::~PLCContext()

{
}

/************************************************************************/
/*                           getOutputLine()                            */
/************************************************************************/

PLCLine *PLCContext::getOutputLine(int line)

{
    CPLAssert( outputDS != NULL );
    
    int  i, width = outputDS->GetRasterXSize();
    PLCLine *lineObj = new PLCLine(width);

    for( i=0; i < outputDS->GetRasterCount(); i++ )
    {
        CPLErr eErr;
        GDALRasterBand *band = outputDS->GetRasterBand(i+1);
        
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

    return lineObj;
}

/************************************************************************/
/*                          writeOutputLine()                           */
/************************************************************************/

void PLCContext::writeOutputLine(int line, PLCLine *lineObj)

{
    CPLAssert( outputDS != NULL );
    
    int  i, width = outputDS->GetRasterXSize();

    for( i=0; i < outputDS->GetRasterCount(); i++ )
    {
        CPLErr eErr;
        GDALRasterBand *band = outputDS->GetRasterBand(i+1);
        
        if( band->GetColorInterpretation() == GCI_AlphaBand )
            eErr = band->RasterIO(GF_Write, 0, line, width, 1, 
                                  lineObj->getAlpha(), width, 1, GDT_Byte,
                                  0, 0);
        else
            eErr = band->RasterIO(GF_Write, 0, line, width, 1, 
                                  lineObj->getBand(i), width, 1, GDT_Int16, 
                                  0, 0);

        if( eErr != CE_None )
            exit(1);
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
