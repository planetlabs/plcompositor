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

#include <algorithm>

#include "compositor.h"

/************************************************************************/
/*                          SameSourceQuality                           */
/************************************************************************/

class SameSourceQuality : public QualityMethodBase 
{
    PLCContext *context;
    double mismatchPenalty; 

public:
    SameSourceQuality() : QualityMethodBase("samesource") {}
    ~SameSourceQuality() {}

    /********************************************************************/
    QualityMethodBase *create(PLCContext* context, WJElement node) {

        SameSourceQuality *obj = new SameSourceQuality();

        obj->context = context;
        obj->mismatchPenalty = WJEDouble(node, "mismatch_penalty", 
                                         WJE_GET, 0.1);
        if( obj->mismatchPenalty < 0.0 || obj->mismatchPenalty > 1.0 )
        {
            CPLError( CE_Fatal, CPLE_AppDefined,
                      "mismatch_penalty=%.3f outside 0.0 to 1.0 range.",
                      obj->mismatchPenalty);
        }

        return obj;
    }

    /********************************************************************/
    int computeQuality(PLCInput *input, PLCLine *lineObj) {
        PLCLine *lastOutputLine = context->getLastOutputLine();
        float *newQuality = lineObj->getNewQuality();

        if( lastOutputLine == NULL )
        {
            for(int i=lineObj->getWidth()-1; i >= 0; i--)
                newQuality[i] = 1.0;
            return TRUE;
        }

        unsigned short *lastSource = lastOutputLine->getSource();
        float singlePenalty = mismatchPenalty / 3.0;
        
        for(int i=lineObj->getWidth()-1; i >= 0; i--)
        {
            float thisQuality = 1.0;

            // compare to top left
            if( i > 0 && lastSource[i-1] != 0
                && lastSource[i-1] != input->getInputIndex()+1 )
                thisQuality -= singlePenalty;

            // compare to top right
            if( i < lineObj->getWidth()-1 && lastSource[i+1] != 0
                && lastSource[i+1] != input->getInputIndex()+1 )
                thisQuality -= singlePenalty;

            // compare to top
            if( lastSource[i] != 0
                && lastSource[i] != input->getInputIndex()+1 )
                thisQuality -= singlePenalty;

            newQuality[i] = thisQuality;
        }
        
        return TRUE;
    }
};

static SameSourceQuality sameSourceQualityTemplateInstance;


