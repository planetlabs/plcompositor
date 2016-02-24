/**
 * Purpose: Source map post processing (sieve filter, etc)
 *
 * Copyright 2016, Planet Labs, Inc.
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
#include "gdal_alg.h"

static void RebuildOutputFromSourceMap(PLCContext *plContext);
static void RebuildOutputLineFromSourceMap(PLCContext *plContext, int line);

/************************************************************************/
/*                         SourcePostProcess()                          */
/************************************************************************/

void SourcePostProcess(PLCContext *plContext)

{
/* -------------------------------------------------------------------- */
/*      Check configuration.                                            */
/* -------------------------------------------------------------------- */
    if( plContext->sourceTraceDS == NULL )
    {
        CPLError( CE_Fatal, CPLE_AppDefined,
                  "source_sieve_threshold set without source trace enabled.");
        exit(1);
    }

    if( plContext->averageBestRatio > 0.0 )
    {
        CPLError( CE_Fatal, CPLE_AppDefined,
                  "source_sieve_threshold and average_best_ratio set, "
                  "they are mutually exclusive.");
        exit(1);
    }

/* -------------------------------------------------------------------- */
/*      Do the sieve filtering.                                         */
/* -------------------------------------------------------------------- */
    GDALRasterBand *poSourceMapBand =
        plContext->sourceTraceDS->GetRasterBand(1);

    CPLErr eErr = GDALSieveFilter(
        (GDALRasterBandH) poSourceMapBand,
        NULL,
        (GDALRasterBandH) poSourceMapBand,
        plContext->sourceSieveThreshold,
        4,
        NULL,
        NULL,
        NULL);

    if( eErr != CE_None )
        exit(1);
    
/* -------------------------------------------------------------------- */
/*      Rebuild the output result image.                                */
/* -------------------------------------------------------------------- */
    RebuildOutputFromSourceMap(plContext);
}

/************************************************************************/
/*                   RebulidOutputLineFromSourceMap()                   */
/************************************************************************/

static void RebuildOutputLineFromSourceMap(PLCContext *plContext,
                                           int line)

{
    PLCLine *lineObj = plContext->getNextOutputLine();
    GByte *dst_alpha = lineObj->getAlpha();
    unsigned int i, iPixel, width=lineObj->getWidth();

    CPLAssert( plContext->line == line );

/* -------------------------------------------------------------------- */
/*      Read the source map for this line.                              */
/* -------------------------------------------------------------------- */
    unsigned short *source = lineObj->getSource();

    CPLErr eErr = plContext->sourceTraceDS->GetRasterBand(1)->RasterIO(
        GF_Read, 0, line, width, 1, 
        source, width, 1, GDT_UInt16, 0, 0);

    if( eErr != CE_None )
        exit( 1 );
    
/* -------------------------------------------------------------------- */
/*      Read inputs.                                                    */
/* -------------------------------------------------------------------- */
    std::vector<PLCLine *> inputLines;

    for(i = 0; i < plContext->inputFiles.size(); i++ )
        inputLines.push_back(plContext->inputFiles[i]->getLine(line));

/* -------------------------------------------------------------------- */
/*      Build output based on source map.                               */
/* -------------------------------------------------------------------- */
    for( iPixel = 0; iPixel < width; iPixel++ )
    {
        if( source[iPixel] == 0 )
            dst_alpha[iPixel] = 0;
        else
        {
            for(int iBand=0; iBand < lineObj->getBandCount(); iBand++)
            {
                float *dst_pixels = lineObj->getBand(iBand);

                dst_pixels[iPixel] = 
                        inputLines[source[iPixel]-1]->getBand(iBand)[iPixel];
            }
            dst_alpha[iPixel] = 255;
        }
    }

/* -------------------------------------------------------------------- */
/*      Cleanup input lines.                                            */
/* -------------------------------------------------------------------- */
    for(i = 0; i < plContext->inputFiles.size(); i++ )
        delete inputLines[i];

/* -------------------------------------------------------------------- */
/*      Write out the image pixels and alpha.                           */
/* -------------------------------------------------------------------- */
    plContext->writeOutputLine(true);
}

/************************************************************************/
/*                     RebulidOutputFromSourceMap()                     */
/************************************************************************/

static void RebuildOutputFromSourceMap(PLCContext *plContext)

{
/* -------------------------------------------------------------------- */
/*      Reset reader.                                                   */
/* -------------------------------------------------------------------- */
    plContext->line = -1;
    
/* -------------------------------------------------------------------- */
/*      Process all lines.                                              */
/* -------------------------------------------------------------------- */
    for(int line=0; line < plContext->outputDS->GetRasterYSize(); line++ )
    {
        RebuildOutputLineFromSourceMap(plContext, line);
    }
}

