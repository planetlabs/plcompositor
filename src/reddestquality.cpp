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
/*                             RedQuality                               */
/************************************************************************/

class RedQuality : public QualityMethodBase 
{
    PLCInput *input;

public:
    RedQuality() : QualityMethodBase("reddest") {}
    ~RedQuality() {}

    /********************************************************************/
    QualityMethodBase *create(PLCContext* context, WJElement node) {

        RedQuality *obj = new RedQuality();
        return obj;
    }

    /********************************************************************/
    int computeQuality(PLCInput *input, PLCLine *lineObj) {

        float *quality = lineObj->getNewQuality();
        int width = lineObj->getWidth();

        if( lineObj->getBandCount() < 3 )
            CPLError( CE_Fatal, CPLE_AppDefined,
                      "Reddest Pixel requested without 3 bands." );

        float *red = lineObj->getBand(0);
        float *green = lineObj->getBand(1);
        float *blue = lineObj->getBand(2);
        GByte *alpha = lineObj->getAlpha();

        for(int i=0; i < width; i++ )
        {
            if( alpha[i] < 128 )
                quality[i] = -1.0;
            else
                quality[i] = red[i] / ((float) red[i]+green[i]+blue[i]+1);
        }        

        return TRUE;
    }
};

static RedQuality redQualityTemplateInstance;
