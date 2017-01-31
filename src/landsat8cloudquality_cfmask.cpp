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
/*                        Landsat8SRCloudQuality()                      */
/*                                                                      */
/*      Compute qualities for this line from the Landsat8 SR quality    */
/*      mask values.                                                    */
/************************************************************************/

class Landsat8CFMaskCloudQuality : public QualityMethodBase
{
    PLCContext *context;
    PLCHistogram cloudHistogram;

    float fully_confident_cloud;
    float mostly_confident_cloud;
    float partially_confident_cloud;
    float not_cloud;

public:
    Landsat8CFMaskCloudQuality() : QualityMethodBase("landsat8_cfmask") {}
    ~Landsat8CFMaskCloudQuality() {}

    /********************************************************************/
    QualityMethodBase *create(PLCContext* context, WJElement node) {

        Landsat8CFMaskCloudQuality *obj = new Landsat8CFMaskCloudQuality();
        obj->context = context;
        obj->cloudHistogram.counts.resize(6);

        obj->fully_confident_cloud = -1.0;
        obj->mostly_confident_cloud = 0.33;
        obj->partially_confident_cloud = 0.66;
        obj->not_cloud = 1.0;

        if( node != NULL )
        {
          obj->fully_confident_cloud = 
              WJEDouble(node, "fully_confident_cloud", WJE_GET, 
                        obj->fully_confident_cloud);
          obj->mostly_confident_cloud = 
              WJEDouble(node, "mostly_confident_cloud", WJE_GET, 
                        obj->mostly_confident_cloud);
          obj->partially_confident_cloud = 
              WJEDouble(node, "partially_confident_cloud", WJE_GET, 
                        obj->partially_confident_cloud);
          obj->not_cloud = 
              WJEDouble(node, "not_cloud", WJE_GET, 
                        obj->not_cloud);
        }

        return obj;
    }

    /********************************************************************/
    int computeQuality(PLCInput *input, PLCLine *lineObj) {

        float *quality = lineObj->getNewQuality();
        unsigned short *cloud = lineObj->getCloud();
        int width = lineObj->getWidth();

        for(int i=0; i < width; i++ )
        {
            switch( cloud[i] ) {

              case 255:
                quality[i] = -1.0;
                break;

              case 3:
                quality[i] = fully_confident_cloud;
                break;

              case 2:
                quality[i] = mostly_confident_cloud;
                break;

              case 1:
                quality[i] = partially_confident_cloud;
                break;

              case 0:
                quality[i] = not_cloud;
                break;
            }
        }

        cloudHistogram.accumulate(quality, width);

        if( context->line == context->height - 1
            && context->verbose > 0 )
            cloudHistogram.report(stdout, "L8 CFMask Cloud Quality");
        return TRUE;
    }
};

static Landsat8CFMaskCloudQuality landsat8CFMaskCloudQualityTemplateInstance;

