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

public:
    DarkQuality() : QualityMethodBase("darkest") {}
    ~DarkQuality() {}

    /********************************************************************/
    QualityMethodBase *create(PLCContext* context, PLCInput* input) {

        DarkQuality *obj = new DarkQuality();
        obj->input = input;
        return obj;
    }

    /********************************************************************/
    int computeQuality(PLCLine *lineObj) {

        float *quality = lineObj->getQuality();
        int width = lineObj->getWidth();

        for(int iBand=0; iBand < lineObj->getBandCount(); iBand++)
        {
            short *pixels = lineObj->getBand(iBand);

            for(int i=0; i < width; i++ )
                quality[i] += (32768 - pixels[i]) / 96000.0;
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

