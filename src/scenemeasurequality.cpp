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
    std::vector<float> measureValues;

public:
    SceneMeasureQuality() : QualityMethodBase("scene_measure") {}
    ~SceneMeasureQuality() {}

    /********************************************************************/
    QualityMethodBase *create(PLCContext* plContext, WJElement node) {

        SceneMeasureQuality *obj = new SceneMeasureQuality();
        obj->initialize(plContext, node);
        return obj;
    }

    
    /********************************************************************/
    void initialize(PLCContext *plContext, WJElement node) {
        if( node == NULL )
        {
            const char *measureName =
                plContext->strategyParams.FetchNameValueDef("scene_measure", 
                                                            "NULL");
            // Do we have rescaling values to try to bring this into 0.0 to 1.0?
            CPLString minPrefix = "scale_min:";
            double scaleMin = 
                atof(plContext->strategyParams.FetchNameValueDef(
                         (minPrefix + measureName).c_str(), "0.0"));
            CPLString maxPrefix = "scale_max:";
            double scaleMax = 
                atof(plContext->strategyParams.FetchNameValueDef(
                         (maxPrefix + measureName).c_str(), "1.0"));

            for( unsigned int iInput=0; 
                 iInput < plContext->inputFiles.size(); iInput++ )
            {
                PLCInput *input = plContext->inputFiles[iInput];
                float measureValue = input->getQM(measureName);
            
                if( measureValue < 0.0 )
                {
                    CPLError( CE_Fatal, CPLE_AppDefined,
                              "Scene %s lacks quality measure %s.", 
                              input->getFilename(),
                              measureName);
                }

                measureValue = (measureValue-scaleMin) / (scaleMax-scaleMin);

                measureValues.push_back(measureValue);
                CPLDebug("PLC", "Using quality %.3f for input %d\n", 
                         measureValue, iInput);
            }
        }
        else
        {
            CPLAssert( FALSE ); // need to implement json support.
        }
    }

    /********************************************************************/
    int computeQuality(PLCInput *input, PLCLine *lineObj) {
        float *quality = lineObj->getNewQuality();
        int width = lineObj->getWidth();
        float measureValue = measureValues[input->getInputIndex()];

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

