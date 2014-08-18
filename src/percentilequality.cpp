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

#include <algorithm>

#include "compositor.h"

/************************************************************************/
/*                          PercentileQuality                           */
/************************************************************************/

class PercentileQuality : public QualityMethodBase 
{
    PLCInput *input;
    std::vector<float> targetQuality;
    double percentileRatio; 

public:
    PercentileQuality() : QualityMethodBase("percentile") {}
    ~PercentileQuality() {}

    /********************************************************************/
    QualityMethodBase *create(PLCContext* context, WJElement node) {
        PercentileQuality *obj = new PercentileQuality();

        if( node == NULL )
        {
            CPLString default_percentile;
            if( EQUAL(context->getStratParam("compositor", "quality"),
                      "median") )
                default_percentile = "50";
            else
                default_percentile = "100";

            // 50 is true median, 100 is best quality, 0 is worst quality.
            obj->percentileRatio = atof(
                context->getStratParam("quality_percentile", 
                                       default_percentile)) / 100.0;

            // median_ratio is here for backwards compatability, quality_percentile
            // Eventually it should be removed.
            if( context->getStratParam("median_ratio", NULL) != NULL )
                obj->percentileRatio = atof(context->getStratParam("median_ratio", NULL));

            CPLDebug("PLC", "Percentile quality in effect, ratio=%.3f",
                     obj->percentileRatio);
        }
        else
        {
            obj->percentileRatio = WJEDouble(node, "quality_percentile", 
                                             WJE_GET, 50.0) / 100.0;
        }
        return obj;
    }

    /********************************************************************/
    void mergeQuality(PLCInput *input, PLCLine *line) {
        float *quality = line->getQuality();
        float *newQuality = line->getNewQuality();

        // In this case we copy the new quality over the old since it already incorporates
        // the old. 
        for(int i=0; i < line->getWidth(); i++)
        {
            quality[i] = newQuality[i];
            newQuality[i] = 1.0;
        }
    }

    /********************************************************************/
    int computeQuality(PLCInput *input, PLCLine *lineObj) {
        float *newQuality = lineObj->getNewQuality();
        float *oldQuality = lineObj->getQuality();

        for(int i=lineObj->getWidth()-1; i >= 0; i--)
        {
            if( oldQuality[i] <= 0 )
                newQuality[i] = -1;
            else
                newQuality[i] = 1.0 - fabs(oldQuality[i] - targetQuality[i]); // rescale?
        }
        
        return TRUE;
    }

    /********************************************************************/
    int computeStackQuality(PLCContext *context, std::vector<PLCLine*>& lines) {

        unsigned int i;
        std::vector<float*> inputQualities;

        targetQuality.resize(context->width);

        for(i = 0; i < context->inputFiles.size(); i++ )
            inputQualities.push_back(lines[i]->getQuality());

        std::vector<float> pixelQualities;
        pixelQualities.resize(context->width);

        for(int iPixel=0; iPixel < context->width; iPixel++)
        {
            int activeCandidates = 0;

            for(i=0; i < inputQualities.size(); i++)
            {
                if( inputQualities[i][iPixel] > 0.0 )
                    pixelQualities[activeCandidates++] = inputQualities[i][iPixel];
            }

            if( activeCandidates > 1 )
            {
                std::sort(pixelQualities.begin(),
                          pixelQualities.begin()+activeCandidates);
                
                int bestCandidate = 
                    MAX(0,MIN(activeCandidates-1,
                              ((int) floor(activeCandidates*percentileRatio))));
                targetQuality[iPixel] = pixelQualities[bestCandidate];
                
            }
            else if( activeCandidates == 1 )
                targetQuality[iPixel] = pixelQualities[0];
            else
                targetQuality[iPixel] = -1.0;
        }

        return QualityMethodBase::computeStackQuality(context, lines);
    }
};

static PercentileQuality percentileQualityTemplateInstance;


