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
/*                             GreenQuality                             */
/************************************************************************/

class GreenQuality : public QualityMethodBase 
{
    PLCInput *input;

public:
    GreenQuality() : QualityMethodBase("greenest") {}
    ~GreenQuality() {}

    /********************************************************************/
    QualityMethodBase *create(PLCContext* context, PLCInput* input) {

        GreenQuality *obj = new GreenQuality();
        obj->input = input;
        return obj;
    }

    /********************************************************************/
    int computeQuality(PLCLine *lineObj) {

        float *quality = lineObj->getQuality();
        int width = lineObj->getWidth();

        if( lineObj->getBandCount() < 3 )
            CPLError( CE_Fatal, CPLE_AppDefined,
                      "Greenest Pixel requested without 3 bands." );

        short *red = lineObj->getBand(0);
        short *green = lineObj->getBand(1);
        short *blue = lineObj->getBand(2);
        GByte *alpha = lineObj->getAlpha();

        for(int i=0; i < width; i++ )
        {
            if( alpha[i] < 128 )
                quality[i] = -1.0;
            else
                quality[i] = green[i] / ((float) red[i]+green[i]+blue[i]+1);
        }        

        return TRUE;
    }
};

static GreenQuality greenQualityTemplateInstance;
