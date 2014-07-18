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
/*                           QualityFromFile                            */
/************************************************************************/

class QualityFromFile : public QualityMethodBase 
{
    PLCContext *context;
    std::vector<GDALDataset*> qualityFiles;
    double scale_min, scale_max;

public:
    QualityFromFile() : QualityMethodBase("qualityfromfile") {}
    ~QualityFromFile() {
        for(unsigned int i=0; i < qualityFiles.size(); i++ )
        {
            GDALClose(qualityFiles[i]);
        }
        qualityFiles.resize(0);
    }

    /********************************************************************/
    QualityMethodBase *create(PLCContext* context, json_object *node) {
        QualityFromFile *obj = new QualityFromFile();
        obj->context = context;
        
        if( node == NULL )
        {
            unsigned int i;
            CPLString file_suffix = context->getStratParam("quality_file", "<missing>");
            
            obj->scale_min = CPLAtof(
                context->getStratParam("quality_file_scale_min", "0.0"));
            obj->scale_max = CPLAtof(
                context->getStratParam("quality_file_scale_max", "1.0"));

            for(i = 0; i < context->inputFiles.size(); i++ )
            {
                CPLString filename = context->inputFiles[i]->getFilename() + file_suffix;

                GDALDataset *ds = (GDALDataset*) GDALOpen(filename, GA_ReadOnly);
                if( ds == NULL )
                {
                    CPLError(CE_Fatal, CPLE_AppDefined,
                             "Failed to open quality file %s.", 
                             filename.c_str());
                }
                obj->qualityFiles.push_back(ds);
            }
        }
        else
        {
            CPLAssert(FALSE);
        }
        
        return obj;
    }

    /********************************************************************/
    int computeQuality(PLCInput *input, PLCLine *lineObj) {

        int width = lineObj->getWidth();
        float *quality = lineObj->getNewQuality();
        CPLErr eErr;
        GDALRasterBand *band = qualityFiles[input->getInputIndex()]->GetRasterBand(1);

        eErr = band->RasterIO(GF_Read, 0, context->line, width, 1, 
                              quality, width, 1, GDT_Float32,
                              0, 0);
        if( eErr != CE_None )
            exit(1);
        
        if( scale_max != 1.0 || scale_min != 0.0)
        {
            for(int i=0; i < width; i++ )
                quality[i] = (quality[i] - scale_min) / (scale_max - scale_min);
        }

        return TRUE;
    }
};

static QualityFromFile qualityFromFileTemplateInstance;

