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
/*                       MedianLineCompositor()                         */
/************************************************************************/

/**
 * \brief Median filtered compositor.
 *
 * This compositor selects the median valued pixel from the stack 
 * based on quality *after* discarding very low quality candidates. 
 */

void MedianLineCompositor(PLCContext *plContext, int line, PLCLine *lineObj)

{
    std::vector<PLCLine *> inputLines;
    unsigned int i, iPixel, width=lineObj->getWidth();
    std::vector<float*> inputQualities;

/* -------------------------------------------------------------------- */
/*      Initialization logic.                                           */
/* -------------------------------------------------------------------- */
    static PLCHistogram activeCandidatesHistogram;
    static float thresholdQuality;

    if( line == 0 )
    {
        activeCandidatesHistogram.scaleMax = plContext->inputFiles.size();
        activeCandidatesHistogram.counts.resize(plContext->inputFiles.size()+2);
        
        thresholdQuality = atof(
            plContext->getStratParam("median_quality_threshold", "0.00001"));
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
    std::vector<int> bestInput(width, -1);
    std::vector<float> bestQuality(width, -1.0); 

    std::vector<InputQualityPair> candidates;
    candidates.resize(plContext->inputFiles.size());

    for(iPixel=0; iPixel < width; iPixel++)
    {
        int activeCandidates = 0;

        for(i = 0; i < plContext->inputFiles.size(); i++ )
        {
            float *quality = inputQualities[i];
            
            if(quality[iPixel] >= thresholdQuality)
            {
                candidates[activeCandidates].inputFile = i;
                candidates[activeCandidates].quality = quality[iPixel];
                activeCandidates++;
            }
        }

        if( activeCandidates > 1 )
            std::sort(candidates.begin(),
                      candidates.begin() + activeCandidates);

        if( activeCandidates > 0 )
        {
            bestInput[iPixel] = activeCandidates/2;
            bestQuality[iPixel] = candidates[bestInput[iPixel]].quality;

            float countAsFloat = activeCandidates;
            activeCandidatesHistogram.accumulate(&countAsFloat, 1);
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

    if( plContext->verbose > 0 
        && line == plContext->outputDS->GetRasterYSize()-1 )
    {
        activeCandidatesHistogram.report(stdout, "Active Candidates");
    }
}
