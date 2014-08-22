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
/*                           LineCompositor()                           */
/************************************************************************/

/**
 * \brief Line compositor.
 *
 * Composite one scanline.  The "quality_percentile" value determines what
 * input pixel to use based on a review of qualities - 100 means highest 
 * quality, and 50 would be median.
 */

void LineCompositor(PLCContext *plContext, int line, PLCLine *lineObj)

{
    std::vector<PLCLine *> inputLines;
    unsigned int i, iPixel, width=lineObj->getWidth();
    std::vector<float*> inputQualities;

/* -------------------------------------------------------------------- */
/*      Read inputs.                                                    */
/* -------------------------------------------------------------------- */
    for(i = 0; i < plContext->inputFiles.size(); i++ )
        inputLines.push_back(plContext->inputFiles[i]->getLine(line));

/* -------------------------------------------------------------------- */
/*      Compute qualities.                                              */
/* -------------------------------------------------------------------- */
    for(unsigned int iQM = 0; iQM < plContext->qualityMethods.size(); iQM++ )
    {
        // TODO(check result status)
        plContext->qualityMethods[iQM]->computeStackQuality(plContext, inputLines);

        if( plContext->isDebugLine(line) )
        {
            for(iPixel=0; iPixel < width; iPixel++)
            {
                if( plContext->isDebugPixel(iPixel, line) )
                {
                    for( i=0; i < inputLines.size(); i++ )
                    {
                        printf( "Input %d quality is %.5f @ %dx%d for quality phase %d.\n", 
                                i+1,
                                inputLines[i]->getNewQuality()[iPixel], 
                                iPixel, line, iQM );
                    }
                }
            }
        }

        // opportunity here to save intermediate "New" quality measures.

        // merge "newQuality()" back into "quality", and reset new Quality.
        for(i = 0; i < inputLines.size(); i++)
        {
            plContext->qualityMethods[iQM]->mergeQuality(
                plContext->inputFiles[i], inputLines[i]);
        }

        if( plContext->isDebugLine(line) )
        {
            for(iPixel=0; iPixel < width; iPixel++)
            {
                if( plContext->isDebugPixel(iPixel, line) )
                {
                    for( i=0; i < inputLines.size(); i++ )
                    {
                        printf( "Input %d quality is %.5f @ %dx%d after merge for quality phase %d.\n", 
                                i+1,
                                inputLines[i]->getQuality()[iPixel], 
                                iPixel, line, iQM );
                    }
                }
            }
        }

    }

    for(i = 0; i < plContext->inputFiles.size(); i++ )
        inputQualities.push_back(inputLines[i]->getQuality());

/* -------------------------------------------------------------------- */
/*      Establish which is the best source for each pixel.              */
/* -------------------------------------------------------------------- */
    unsigned short *bestInput = lineObj->getSource();
    float *bestQuality = lineObj->getQuality();

    for(iPixel=0; iPixel < width; iPixel++)
    {
        bestQuality[iPixel] = 0.0;
        bestInput[iPixel] = 0;

        for(i = 0; i < plContext->inputFiles.size(); i++ )
        {
            float *quality = inputQualities[i];

            if(quality[iPixel] > bestQuality[iPixel])
            {
                bestQuality[iPixel] = quality[iPixel];
                bestInput[iPixel] = i+1;
            }
        }

        if( bestInput[iPixel] != 0 )
        {
            if( plContext->isDebugPixel(iPixel, line) )
                printf("No active candidates @ %d,%d\n", 
                       iPixel, line );
        }
        else
        {
            if( plContext->isDebugPixel(iPixel, line) )
                printf("Best quality for %d,%d is %.5f from input %d.\n",
                       iPixel, line, bestQuality[iPixel], bestInput[iPixel]);
        }
    }

    plContext->qualityHistogram.accumulate(bestQuality, width);

/* -------------------------------------------------------------------- */
/*      Build output with best pixels source for each pixel.            */
/* -------------------------------------------------------------------- */
    for(iPixel = 0; iPixel < width; iPixel++) 
    {
        GByte *dst_alpha = lineObj->getAlpha();

        if(bestInput[iPixel] != 0)
        {
            for(int iBand=0; iBand < lineObj->getBandCount(); iBand++)
            {
                short *dst_pixels = lineObj->getBand(iBand);
                short *src_pixels = 
                    inputLines[bestInput[iPixel]-1]->getBand(iBand);
                dst_pixels[iPixel] = src_pixels[iPixel];
            }
            dst_alpha[iPixel] = 255;
        }
        else
            dst_alpha[iPixel] = 0;
    }

/* -------------------------------------------------------------------- */
/*      Consider writing input qualities.                               */
/* -------------------------------------------------------------------- */
    if( plContext->qualityDS != NULL )
    {
        for(i = 0; i < plContext->inputFiles.size(); i++ )
            plContext->qualityDS->GetRasterBand(i+2)->
                RasterIO(GF_Write, 0, line, width, 1, 
                         inputLines[i]->getQuality(), width, 1, GDT_Float32, 
                         0, 0);
    }

/* -------------------------------------------------------------------- */
/*      Cleanup input buffers.                                          */
/* -------------------------------------------------------------------- */
    for(i = 0; i < plContext->inputFiles.size(); i++ )
        delete inputLines[i];
}
