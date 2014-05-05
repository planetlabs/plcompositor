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
/*                         SceneMeasureQuality                          */
/************************************************************************/

class SceneMeasureQuality : public QualityMethodBase 
{
    PLCInput *input;
    float measureValue;

public:
    SceneMeasureQuality() : QualityMethodBase("scene_measure") {}
    ~SceneMeasureQuality() {}

    /********************************************************************/
    QualityMethodBase *create(PLCContext* plContext, PLCInput* input) {

        SceneMeasureQuality *obj = new SceneMeasureQuality();
        obj->initialize(plContext, input);
        return obj;
    }

    
    /********************************************************************/
    void initialize(PLCContext *plContext, PLCInput* input) {
        input = input;

        const char *measureName =
            plContext->strategyParams.FetchNameValueDef("scene_measure", 
                                                        "NULL");
        float direction = 1.0;
        if( measureName[0] == '-' )
        {
            measureName++;
            direction = -1.0;
        }

        measureValue = input->getQM(measureName);

        if( measureValue < 0.0 )
        {
            CPLError( CE_Fatal, CPLE_AppDefined,
                      "Scene %s lacks quality measure %s.", 
                      input->getFilename(),
                      measureName);
        }

        if( direction < 0.0 )
            measureValue = 100000000.0 - measureValue;
    }

    /********************************************************************/
    int computeQuality(PLCLine *lineObj) {

        float *quality = lineObj->getQuality();
        int width = lineObj->getWidth();

        for(int iBand=0; iBand < lineObj->getBandCount(); iBand++)
        {
            for(int i=0; i < width; i++ )
                quality[i] = measureValue;
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

static SceneMeasureQuality sceneMeasureQualityTemplateInstance;

