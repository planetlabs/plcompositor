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
/*                             DarkQuality                             */
/************************************************************************/

class DarkQuality : public QualityMethodBase 
{
    PLCInput *input;
    double    scale_min;
    double    scale_max;

public:
    DarkQuality() : QualityMethodBase("darkest") {}
    ~DarkQuality() {}

    /********************************************************************/
    QualityMethodBase *create(PLCContext* context, WJElement node) {
        DarkQuality *obj = new DarkQuality();

        // We default to 256 as scale_max to ensure that saturated pixels
        // do not result in quality zero which is transparent.
        obj->scale_min = 0.0;
        obj->scale_max = 256.0;

        if( node != NULL )
        {
            obj->scale_min = WJEDouble(node, "scale_min", WJE_GET, 
                                       obj->scale_min);
            obj->scale_max = WJEDouble(node, "scale_max", WJE_GET, 
                                       obj->scale_max);
        }

        return obj;
    }

    /********************************************************************/
    int computeQuality(PLCInput *input, PLCLine *lineObj) {

        float *quality = lineObj->getNewQuality();
        int width = lineObj->getWidth();
        double scale = 1.0 / (lineObj->getBandCount() * (scale_max-scale_min));

        memset(quality, 0, sizeof(float) * width);
        for(int iBand=0; iBand < lineObj->getBandCount(); iBand++)
        {
            float *pixels = lineObj->getBand(iBand);

            for(int i=0; i < width; i++ )
                quality[i] += (scale_max - pixels[i]) * scale;
        }
    
        GByte *alpha = lineObj->getAlpha();
        for(int i=0; i < width; i++ )
        {
            if( alpha[i] < 128 )
                quality[i] = -1.0;
        }

        return TRUE;
    }
};

static DarkQuality darkQualityTemplateInstance;

