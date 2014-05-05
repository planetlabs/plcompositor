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
/*                            PLCHistogram()                            */
/************************************************************************/

PLCHistogram::PLCHistogram() :
        scaleMin(0.0),
        scaleMax(1.0),
        actualMean(0.0),
        actualCount(0)

{
}

/************************************************************************/
/*                           ~PLCHistogram()                            */
/************************************************************************/

PLCHistogram::~PLCHistogram()

{
}

/************************************************************************/
/*                             accumulate()                             */
/************************************************************************/

void PLCHistogram::accumulate(float *quality, int count)

{
    if( count == 0 )
        return;

    if( actualCount == 0 )
    {
        actualMin = actualMax = quality[0];
        counts.resize(66);
    }

    double addToMean = 0.0;
    for( int i = 0; i < count; i++ )
    {
        actualMin = MIN(actualMin,quality[i]);
        actualMax = MAX(actualMax,quality[i]);
        addToMean += quality[i];

        if( quality[i] < scaleMin )
            counts[0]++;
        else if (quality[i] > scaleMax )
            counts[counts.size()-1]++;
        else
        {
            int iBucket = (int) 
                floor((counts.size()-2) * (quality[i] - scaleMin) 
                      / (scaleMax-scaleMin));

            counts[iBucket]++;
        }
    }

    actualMean = (actualMean * actualCount + addToMean) / (actualCount + count);
    actualCount += count;
}

/************************************************************************/
/*                               report()                               */
/************************************************************************/

void PLCHistogram::report(FILE *fp, const char *id)

{
    if( actualCount == 0 )
    {
        fprintf(fp, "%s histogram is empty.\n", id);
        return;
    }

    // Figure out scaling for display, deliberately exclude outliers.
    int maxWithinHistogram = 0;
    for( unsigned int i = 1; i < counts.size()-1; i++ )
        maxWithinHistogram = MAX(maxWithinHistogram, counts[i]);
    maxWithinHistogram = MAX(1,maxWithinHistogram);

    fprintf(fp, "\n\n%s Histogram and Stats\n", id);

    for( unsigned int i = 0; i < counts.size(); i++ )
    {
        CPLString label;

        if( i == 0 )
            label = "underflow";
        else if( i == 1)
            label.Printf("%.2f", scaleMin);
        else if( i == counts.size()-2 )
            label.Printf("%.2f", scaleMax);
        else if( i == counts.size()-1 )
            label = "overflow";

        fprintf(fp, "%9s |", label.c_str());
        
        int starCount = (int) ceil(counts[i] * 60.0 / maxWithinHistogram);
        starCount = MIN(starCount, 60);
        
        for( int j = 0; j < starCount; j++ )
            fputc('*', fp);
        if( counts[i] > maxWithinHistogram )
            fputc('+', fp);
        fprintf( fp, "\n" );
    }

    fprintf(fp, "Count=%d, Min=%.2f, Max=%.2f, Mean=%.2f\n",
            actualCount, actualMin, actualMax, actualMean);
    fprintf(fp, "\n" );
}
