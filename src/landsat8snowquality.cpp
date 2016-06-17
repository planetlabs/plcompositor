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
/*                        Landsat8SnowQuality()                        */
/*                                                                      */
/*      Compute qualities for this line from the Landsat8 snow mask    */
/*      values.                                                         */
/************************************************************************/

/*
 * See landsat8cloudquality.cpp for details on the bitmask in the L8 quality
 * band
 */

class Landsat8SnowQuality : public QualityMethodBase
{
    PLCContext *context;
    PLCHistogram snowHistogram;

    float fully_confident_snow;
    float mostly_confident_snow;
    float partially_confident_snow;
    float not_snow;

public:
    Landsat8SnowQuality() : QualityMethodBase("landsat8snow") {}
    ~Landsat8SnowQuality() {}

    /********************************************************************/
    QualityMethodBase *create(PLCContext* context, WJElement node) {

        Landsat8SnowQuality *obj = new Landsat8SnowQuality();
        obj->context = context;
        obj->snowHistogram.counts.resize(6);

        obj->fully_confident_snow = -1.0;
        obj->mostly_confident_snow = 0.33;
        obj->partially_confident_snow = 0.66;
        obj->not_snow = 1.0;

        if( node != NULL )
        {
            obj->fully_confident_snow =
                WJEDouble(node, "fully_confident_snow", WJE_GET,
                          obj->fully_confident_snow);
            obj->mostly_confident_snow =
                WJEDouble(node, "mostly_confident_snow", WJE_GET,
                          obj->mostly_confident_snow);
            obj->partially_confident_snow =
                WJEDouble(node, "partially_confident_snow", WJE_GET,
                          obj->partially_confident_snow);
            obj->not_snow =
                WJEDouble(node, "not_snow", WJE_GET,
                          obj->not_snow);
        }

        return obj;
    }

    /********************************************************************/
    int computeQuality(PLCInput *input, PLCLine *lineObj) {

        float *quality = lineObj->getNewQuality();
        unsigned short *snow = lineObj->getCloud();
        int width = lineObj->getWidth();

        for(int i=0; i < width; i++ )
        {
            // Snow bits
            switch( snow[i] & 0x0c00 ) {
              case 0x0c00:
                quality[i] = fully_confident_snow;
                break;

              case 0x0800:
                quality[i] = mostly_confident_snow;
                break;

              case 0x0400:
                quality[i] = partially_confident_snow;
                break;

              default:
                if( snow[i] & 0x7 ) // dead pixel markers.
                    quality[i] = -1.0;
                else
                    quality[i] = not_snow;
                break;
            }
        }

        snowHistogram.accumulate(quality, width);

        if( context->line == context->height - 1
            && context->verbose > 0 )
            snowHistogram.report(stdout, "L8 Snow Quality");
        return TRUE;
    }
};

static Landsat8SnowQuality landsat8SnowQualityTemplateInstance;

