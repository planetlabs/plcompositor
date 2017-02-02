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
/*                         Landsat8CFMaskQuality()                      */
/*                                                                      */
/*      Compute qualities for this line from the Landsat8 CFMask        */
/*      quality mask values.                                            */
/************************************************************************/

class Landsat8CFMaskQuality : public QualityMethodBase
{
    PLCContext *context;
    PLCHistogram cloudHistogram;

    float clear;
    float water;
    float cloud_shadow;
    float snow;
    float cloud;

public:
    Landsat8CFMaskQuality() : QualityMethodBase("landsat8_cfmask") {}
    ~Landsat8CFMaskQuality() {}

    /********************************************************************/
    QualityMethodBase *create(PLCContext* context, WJElement node) {

        Landsat8CFMaskQuality *obj = new Landsat8CFMaskQuality();
        obj->context = context;
        obj->cloudHistogram.counts.resize(6);

        obj->clear = 1.0;
        obj->water = 1.0;
        obj->cloud_shadow = 0.01;
        obj->snow = 1.0;
        obj->cloud = 0.01;

        if( node != NULL )
        {
          obj->clear =
              WJEDouble(node, "clear", WJE_GET,
                        obj->clear);
          obj->water = 
              WJEDouble(node, "water", WJE_GET,
                        obj->water);
          obj->cloud_shadow =
              WJEDouble(node, "cloud_shadow", WJE_GET,
                        obj->cloud_shadow);
          obj->snow = 
              WJEDouble(node, "snow", WJE_GET,
                        obj->snow);
          obj->cloud = 
              WJEDouble(node, "cloud", WJE_GET,
                        obj->cloud);
        }

        return obj;
    }

    /********************************************************************/
    int computeQuality(PLCInput *input, PLCLine *lineObj) {

        float *quality = lineObj->getNewQuality();
        unsigned short *value = lineObj->getCloud();
        int width = lineObj->getWidth();

        for(int i=0; i < width; i++ )
        {
            switch( value[i] ) {

              case 255:
                quality[i] = -1.0;
                break;

              case 0:
                quality[i] = clear;
                break;

              case 1:
                quality[i] = water;
                break;

              case 2:
                quality[i] = cloud_shadow;
                break;

              case 3:
                quality[i] = snow;
                break;

              case 4:
                quality[i] = cloud;
                break;
            }
        }

        cloudHistogram.accumulate(quality, width);

        if( context->line == context->height - 1
            && context->verbose > 0 )
            cloudHistogram.report(stdout, "L8 CFMask Quality");
        return TRUE;
    }
};

static Landsat8CFMaskQuality landsat8CFMaskQualityTemplateInstance;

