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

class Landsat8SRCloudQuality : public QualityMethodBase
{
    PLCContext *context;
    PLCHistogram cloudHistogram;

    float cirrus;
    float cloud;
    float shadow;
    float adjacent;
    float climatology_level_aerosol;
    float low_aerosol;
    float average_aerosol;
    float high_aerosol;
    float not_cloud;

public:
    Landsat8SRCloudQuality() : QualityMethodBase("landsat8sr") {}
    ~Landsat8SRCloudQuality() {}

    /********************************************************************/
    QualityMethodBase *create(PLCContext* context, WJElement node) {

        Landsat8SRCloudQuality *obj = new Landsat8SRCloudQuality();
        obj->context = context;
        obj->cloudHistogram.counts.resize(6);

        obj->cirrus = -1.0;
        obj->cloud = -1.0;
        obj->shadow = 0.33;
        obj->adjacent = 0.33;
        obj->climatology_level_aerosol = 1.0;
        obj->low_aerosol = 0.75;
        obj->average_aerosol = 0.5;
        obj->high_aerosol = -1.0;
        obj->not_cloud = 1.0;

        if( node != NULL )
        {
            obj->cirrus =
                WJEDouble(node, "cirrus", WJE_GET,
                          obj->cirrus);
            obj->cloud =
                WJEDouble(node, "cloud", WJE_GET,
                          obj->cloud);
            obj->shadow =
                WJEDouble(node, "shadow", WJE_GET,
                          obj->shadow);
            obj->adjacent =
                WJEDouble(node, "adjacent", WJE_GET,
                          obj->adjacent);
            obj->climatology_level_aerosol =
                WJEDouble(node, "climatology_level_aerosol", WJE_GET,
                          obj->climatology_level_aerosol);
            obj->low_aerosol =
                WJEDouble(node, "low_aerosol", WJE_GET,
                          obj->low_aerosol);
            obj->average_aerosol =
                WJEDouble(node, "average_aerosol", WJE_GET,
                          obj->average_aerosol);
            obj->high_aerosol =
                WJEDouble(node, "high_aerosol", WJE_GET,
                          obj->high_aerosol);
            obj->not_cloud = 
                WJEDouble(node, "not_cloud", WJE_GET, 
                          obj->not_cloud);

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
            if ( value[i] == 0 ) {
                // Always skip pixels at edges with nodata cloud values
                quality[i] = -1.0;
            } else if ( value[i] & 0x2 ) {
                quality[i] = cloud;
            } else {
                quality[i] = not_cloud;
            }

            if ( value[i] & 0x1 ) 
                quality[i] *= cirrus;
            if ( value[i] & 0x4 ) 
                quality[i] *= adjacent;
            if ( value[i] & 0x8 ) 
                quality[i] *= shadow;

            // Aerosol values take up two bits.
            switch ( (value[i] & 0x10) + 2 * (value[i] & 0x20) ) {
                case 0:
                    quality[i] *= climatology_level_aerosol;
                    break;
                case 1:
                    quality[i] *= low_aerosol;
                    break;
                case 2:
                    quality[i] *= average_aerosol;
                    break;
                case 3:
                    quality[i] *= high_aerosol;
                    break;
            }
        }

        cloudHistogram.accumulate(quality, width);

        if( context->line == context->height - 1
            && context->verbose > 0 )
            cloudHistogram.report(stdout, "L8 SR Cloud Quality");
        return TRUE;
    }
};

static Landsat8SRCloudQuality landsat8SRCloudQualityTemplateInstance;

