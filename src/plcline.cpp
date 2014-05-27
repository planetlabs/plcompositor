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
/*                              PLCLine()                               */
/************************************************************************/

PLCLine::PLCLine(int width)

{
    this->width = width;
    bandCount = 0;
    cloud = NULL;
    source = NULL;
    alpha = NULL;
    quality = NULL;
    cloudQuality = NULL;
}

/************************************************************************/
/*                              ~PLCLine()                              */
/************************************************************************/

PLCLine::~PLCLine()

{
    for(int i=0; i < bandCount; i++)
        CPLFree(bandData[i]);

    CPLFree( cloud );
    CPLFree( source );
    CPLFree( alpha );
    CPLFree( quality );
    CPLFree( cloudQuality );
}

/************************************************************************/
/*                              getBand()                               */
/************************************************************************/

short *PLCLine::getBand(int band)
{
    if( band == bandCount )
    {
        bandCount++;
        bandData.push_back((short *) CPLCalloc(sizeof(short),width));
        return bandData[band];
    }

    if( band < 0 || band >= bandCount )
        CPLError(CE_Fatal, CPLE_AppDefined,
                 "Band %d requested, but only %d bands available.", 
                 band, bandCount );

    return bandData[band];
}

/************************************************************************/
/*                              getAlpha()                              */
/************************************************************************/

GByte *PLCLine::getAlpha()

{
    if( alpha == NULL )
    {
        alpha = (GByte *) CPLMalloc(width);
        memset(alpha, 255, width);
    }

    return alpha;
}

/************************************************************************/
/*                              getCloud()                              */
/************************************************************************/

unsigned short *PLCLine::getCloud()

{
    if( cloud == NULL )
    {
        cloud = (unsigned short *) CPLCalloc(sizeof(short),width);
    }

    return cloud;
}

/************************************************************************/
/*                             getSource()                              */
/************************************************************************/

unsigned short *PLCLine::getSource()

{
    if( source == NULL )
    {
       source = (unsigned short *) CPLCalloc(sizeof(short),width);
    }

    return source;
}

/************************************************************************/
/*                             getQuality()                             */
/************************************************************************/

float *PLCLine::getQuality()

{
    if( quality == NULL )
    {
        quality = (float *) CPLCalloc(sizeof(float),width);
    }

    return quality;
}

/************************************************************************/
/*                          getCloudQuality()                           */
/************************************************************************/

float *PLCLine::getCloudQuality()

{
    if( cloudQuality == NULL )
    {
        cloudQuality = (float *) CPLCalloc(sizeof(float),width);
        for( int i=0; i < width; i++ )
            cloudQuality[i] = 1.0;
    }

    return cloudQuality;
}

/************************************************************************/
/*                         mergeCloudQuality()                          */
/************************************************************************/

void PLCLine::mergeCloudQuality()

{
    getQuality();
    getCloudQuality();

    for(int i=0; i < width; i++)
    {
        if( cloudQuality[i] < 0.0 )
            quality[i] = -1.0;
        else
            quality[i] = quality[i] * cloudQuality[i];
    }
}
