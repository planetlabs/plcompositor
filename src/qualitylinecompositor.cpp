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
/*                       QualityLineCompositor()                        */
/************************************************************************/

void QualityLineCompositor(PLCContext *plContext, int line, PLCLine *lineObj)

{
    std::vector<PLCLine *> inputLines;
    unsigned int i, iPixel, width=lineObj->getWidth();

/* -------------------------------------------------------------------- */
/*      Read inputs and compute qualities.                              */
/* -------------------------------------------------------------------- */
    for(i = 0; i < plContext->inputFiles.size(); i++ )
    {
        inputLines.push_back(plContext->inputFiles[i]->getLine(line));
        plContext->inputFiles[i]->computeQuality(plContext, inputLines[i]);
    }

/* -------------------------------------------------------------------- */
/*      Establish which is the best source for each pixel.              */
/* -------------------------------------------------------------------- */
    std::vector<int> bestInput(width, -1);
    std::vector<float> bestQuality(width, -1.0); 

    for(i = 0; i < plContext->inputFiles.size(); i++ )
    {
        float *quality = inputLines[i]->getQuality();

        for(iPixel=0; iPixel < width; iPixel++) 
        {
            if(quality[iPixel] > bestQuality[iPixel])
            {
                bestQuality[iPixel] = quality[iPixel];
                bestInput[iPixel] = i;
            }
            
            if( plContext->isDebugPixel(iPixel, line) )
                printf( "Quality from input %d @ %d,%d is %.8f\n", 
                        i, iPixel, line, quality[iPixel] );
        }
    }

    plContext->qualityHistogram.accumulate(bestQuality.data(), width);

/* -------------------------------------------------------------------- */
/*      Build output with best pixels source for each pixel.            */
/* -------------------------------------------------------------------- */
    for(iPixel = 0; iPixel < width; iPixel++) 
    {
        GByte *dst_alpha = lineObj->getAlpha();

        if(bestInput[iPixel] != -1)
        {
            for(int iBand=0; iBand < lineObj->getBandCount(); iBand++)
            {
                short *dst_pixels = lineObj->getBand(iBand);
                short *src_pixels = inputLines[bestInput[iPixel]]->getBand(iBand);
                dst_pixels[iPixel] = src_pixels[iPixel];

                if( plContext->isDebugPixel(iPixel, line) )
                    printf( "Assign band %d value %d for pixel %d,%d from source %d.\n", 
                            iBand, dst_pixels[iPixel], iPixel, line, 
                            bestInput[iPixel] );
            }
            dst_alpha[iPixel] = 255;
        }
        else
            dst_alpha[iPixel] = 0;
    }

/* -------------------------------------------------------------------- */
/*      Cleanup input buffers.                                          */
/* -------------------------------------------------------------------- */
    for(i = 0; i < plContext->inputFiles.size(); i++ )
        delete inputLines[i];
}
