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
#include <algorithm>

// used to sort for median.
class InputQualityPair {
public:
    int inputFile;
    float quality;
    
    bool operator< (const InputQualityPair &rhs) const {
        return this->quality < rhs.quality;
    }
};

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
/*      Initialization logic.                                           */
/* -------------------------------------------------------------------- */
    static PLCHistogram activeCandidatesHistogram;
    static float thresholdQuality;
    static float medianRatio;

    if( line == 0 )
    {
        activeCandidatesHistogram.scaleMax = plContext->inputFiles.size();
        activeCandidatesHistogram.counts.resize(plContext->inputFiles.size()+2);
        
        thresholdQuality = atof(
            plContext->getStratParam("quality_threshold", "0.00001"));

        CPLString default_percentile;
        if( EQUAL(plContext->getStratParam("compositor", "quality"),
                  "quality") )
            default_percentile = "100";
        else
            default_percentile = "50";

        // 50 is true median, 100 is best quality, 0 is worst quality.
        medianRatio = atof(
            plContext->getStratParam("quality_percentile", 
                                     default_percentile)) / 100.0;

        // median_ratio is here for backwards compatability, quality_percentile
        // Eventually it should be removed.
        if( plContext->getStratParam("median_ratio", NULL) != NULL )
            medianRatio = atof(
                plContext->getStratParam("median_ratio", NULL));
    }

/* -------------------------------------------------------------------- */
/*      Read inputs and compute qualities.                              */
/* -------------------------------------------------------------------- */
    for(i = 0; i < plContext->inputFiles.size(); i++ )
    {
        inputLines.push_back(plContext->inputFiles[i]->getLine(line));
        plContext->inputFiles[i]->computeQuality(plContext, inputLines[i]);
        inputQualities.push_back(inputLines[i]->getQuality());
    }

/* -------------------------------------------------------------------- */
/*      Establish which is the best source for each pixel.              */
/* -------------------------------------------------------------------- */
    unsigned short *bestInput = lineObj->getSource();
    float *bestQuality = lineObj->getQuality();

    std::vector<InputQualityPair> candidates;
    candidates.resize(plContext->inputFiles.size());

    for(iPixel=0; iPixel < width; iPixel++)
    {
        int activeCandidates = 0;

        bestQuality[iPixel] = -1.0;

        for(i = 0; i < plContext->inputFiles.size(); i++ )
        {
            float *quality = inputQualities[i];

            if(quality[iPixel] >= thresholdQuality)
            {
                candidates[activeCandidates].inputFile = i;
                candidates[activeCandidates].quality = quality[iPixel];
                activeCandidates++;
            }
            
            if( plContext->isDebugPixel(iPixel, line) )
                printf( "Quality from input %d @ %d,%d is %.8f\n", 
                        i, iPixel, line, quality[iPixel] );
        }

        // TODO: we could just pick the "max" if medianRatio is 1.0 and
        // optimize out the sorting, and tracking candidates. 
        if( activeCandidates > 1 )
            std::sort(candidates.begin(),
                      candidates.begin() + activeCandidates);

        if( activeCandidates > 0 )
        {
            int bestCandidate = 
                MAX(0,MIN(activeCandidates-1,
                          ((int) floor(activeCandidates*medianRatio))));
            bestInput[iPixel] = candidates[bestCandidate].inputFile + 1;
            bestQuality[iPixel] = candidates[bestCandidate].quality;

            float countAsFloat = activeCandidates;
            activeCandidatesHistogram.accumulate(&countAsFloat, 1);

            if( plContext->isDebugPixel(iPixel, line) )
                printf( "Selected quality %.8f from input %d @ %d,%d from %d active candidates.\n", 
                        bestQuality[iPixel], bestInput[iPixel]-1, iPixel, line,
                        activeCandidates );
        }
        else
        {
            if( plContext->isDebugPixel(iPixel, line) )
                printf("No active candidates @ %d,%d\n", 
                       iPixel, line );
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

    if( plContext->verbose > 0 
        && line == plContext->outputDS->GetRasterYSize()-1 )
    {
        activeCandidatesHistogram.report(stdout, "Active Candidates");
    }
}
