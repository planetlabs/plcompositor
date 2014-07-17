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
/*                        Landsat8CloudQuality()                        */
/*                                                                      */
/*      Compute qualities for this line from the Landsat8 cloud mask    */
/*      values.                                                         */
/************************************************************************/

/*

From: https://landsat.usgs.gov/L8QualityAssessmentBand.php

For the single bits (0, 1, 2, and 3):

0 = No, this condition does not exist
1 = Yes, this condition exists.

The double bits (4-5, 6-7, 8-9, 10-11, 12-13, and 14-15) represent levels of confidence that a condition exists:

00 = Algorithm did not determine the status of this condition
01 = Algorithm has low confidence that this condition exists 
     (0-33 percent confidence)
10 = Algorithm has medium confidence that this condition exists 
     (34-66 percent confidence)
11 = Algorithm has high confidence that this condition exists 
     (67-100 percent confidence).

 Mask    Meaning
0x0001 - Designated Fill0
0x0002 - Dropped Frame
0x0004 - Terrain Occlusion
0x0008 - Reserved
0x0030 - Water Confidence
0x00c0 - Reserved for cloud shadow
0x0300 - Vegitation confidence
0x0c00 - Show/ice Confidence
0x3000 - Cirrus Confidence
0xc000 - Cloud Confidence

*/

class Landsat8CloudQuality : public QualityMethodBase 
{
    PLCContext *context;
    PLCHistogram cloudHistogram;

public:
    Landsat8CloudQuality() : QualityMethodBase("landsat8") {}
    ~Landsat8CloudQuality() {}

    /********************************************************************/
    QualityMethodBase *create(PLCContext* context, json_object *node) {

        Landsat8CloudQuality *obj = new Landsat8CloudQuality();
        obj->context = context;
        obj->cloudHistogram.counts.resize(6);
        return obj;
    }

    /********************************************************************/
    int computeQuality(PLCInput *input, PLCLine *lineObj) {

        float *quality = lineObj->getNewQuality();
        unsigned short *cloud = lineObj->getCloud();
        int width = lineObj->getWidth();

        float ALL_CLOUD = -1.0; // TODO(warmerdam): add flag to set 
        // zero for opaque last resort cloud pixels.
        static const float MOSTLY_CLOUD = 0.33;
        static const float PARTIAL_CLOUD = 0.66;
        static const float NO_CLOUD = 1.0;
    
        for(int i=0; i < width; i++ )
        {
            switch( cloud[i] & 0xc000 ) {
              case 0xc000:
                quality[i] = ALL_CLOUD;
                break;

              case 0x8000:
                quality[i] = MOSTLY_CLOUD;
                break;
            
              case 0x4000:
                quality[i] = PARTIAL_CLOUD;
                break;

              default:
                if( cloud[i] & 0x7 ) // dead pixel markers.
                    quality[i] = -1.0;
                else
                    quality[i] = NO_CLOUD;
                break;
            }
        }

        cloudHistogram.accumulate(quality, width);

        if( context->line == context->height - 1 
            && context->verbose > 0 )
            cloudHistogram.report(stdout, "L8 Cloud Quality");
        return TRUE;
    }
};

static Landsat8CloudQuality landsat8CloudQualityTemplateInstance;

