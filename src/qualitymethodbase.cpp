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

static std::map<CPLString,QualityMethodBase*> *templateMap = NULL;
    
/************************************************************************/
/*                         QualityMethodBase()                          */
/************************************************************************/

QualityMethodBase::QualityMethodBase(const char *name)

{
    this->name = name;

    if( templateMap == NULL )
        templateMap = new std::map<CPLString,QualityMethodBase*>();

    if( templateMap->count(this->name) == 0)
        (*templateMap)[this->name] = this;
}

/************************************************************************/
/*                            ~QualityMethodBase()                      */
/************************************************************************/

QualityMethodBase::~QualityMethodBase()

{
}

/************************************************************************/
/*                       CreateQualityFunction()                        */
/************************************************************************/

QualityMethodBase *QualityMethodBase::CreateQualityFunction(
    PLCContext *context, WJElement node, const char *name_in)

{
    CPLString name(name_in);

    if( templateMap->count(name) == 0 )
    {
        CPLError( CE_Fatal, CPLE_AppDefined,
                  "Failed to find quality function with name '%s'.",
                  name.c_str() );
    }

    return (*templateMap)[name]->create(context, node);
}

/************************************************************************/
/*                        computeStackQuality()                         */
/*                                                                      */
/*      Default stack quality, just invoke the per-input quality.       */
/************************************************************************/

int QualityMethodBase::computeStackQuality(PLCContext *context,
                                           std::vector<PLCLine *> &lines)

{
    int result = TRUE;
    
    for( unsigned int iInput=0; iInput < lines.size(); iInput++)
    {
        if( !computeQuality(context->inputFiles[iInput], lines[iInput]) )
            result = FALSE;
    }

    return result;
}

/************************************************************************/
/*                            mergeQuality()                            */
/*                                                                      */
/*      Default multiplicative merge of new quality into old quality.   */
/************************************************************************/

void QualityMethodBase::mergeQuality(PLCInput *input, PLCLine *line)

{
    float *quality = line->getQuality();
    float *newQuality = line->getNewQuality();

    for(int i=0; i < line->getWidth(); i++)
    {
        if( newQuality[i] < 0.0 || quality[i] < 0.0 )
            quality[i] = -1.0;
        else
            quality[i] = quality[i] * newQuality[i];

        // reset new quality to default.
        newQuality[i] = 1.0;
    }
}
