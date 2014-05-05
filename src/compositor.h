/**
 * Purpose: Core Pixel Lapse Compositor include file.
 *
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

#include "gdal_priv.h"
#include <map>

class QualityMethodBase;
class PLCContext;

class PLCLine {
    int     width;
    
    int     bandCount;
    std::vector<short*> bandData;
    unsigned short  *cloud;
    GByte  *alpha;
    float  *quality;
    float  *cloudQuality;
    
  public:
    PLCLine(int width);
    virtual ~PLCLine();

    int     getWidth() { return width; }
    int     getBandCount() { return bandCount; }
    short  *getBand(int);
    GByte  *getAlpha();
    unsigned short  *getCloud();
    float  *getQuality();
    float  *getCloudQuality();

    void    mergeCloudQuality();
};

class PLCHistogram {
  public:
    PLCHistogram();
    ~PLCHistogram();
    
    double scaleMin;
    double scaleMax;

    double actualMin;
    double actualMax;
    double actualMean;
    int    actualCount;
    
    std::vector<int> counts;

    void accumulate(float *qualities, int count);
    void report(FILE *fp, const char *id);
};


class PLCInput {
    CPLString    filename;
    GDALDataset *DS;
    
    CPLString    cloudMask;
    GDALDataset *cloudDS;
    
    std::map <CPLString,double> qualityMetrics;

    PLCHistogram qualityHistogram;
    PLCHistogram cloudQualityHistogram;
    
    QualityMethodBase *qualityMethod;
    QualityMethodBase *cloudQualityMethod;
  public:
                 PLCInput();
    virtual     ~PLCInput();

    int          ConsumeArgs(int argc, char **argv);
    void         Initialize(PLCContext *);

    double       getQM(const char *key, double defaultValue = -1.0);

    const char  *getFilename() { return filename; }
    GDALDataset *getDS();

    const char  *getCloudFilename() { return cloudMask; }
    GDALDataset *getCloudDS();

    PLCLine     *getLine(int line);

    int          computeQuality(PLCContext*, PLCLine*);
};

class PLCContext {
  public:
    PLCContext();
    virtual ~PLCContext();

    int           quiet;
    int           verbose;
    
    CPLStringList strategyParams;

    CPLString     outputFilename;
    GDALDataset  *outputDS;

    std::vector<PLCInput*> inputFiles;

    PLCLine *     getOutputLine(int line);
    void          writeOutputLine(int line, PLCLine *);

    PLCHistogram  qualityHistogram;
};

class QualityMethodBase {
  protected:
    QualityMethodBase(const char *name);
    CPLString name;

  public:
    virtual ~QualityMethodBase();

    virtual QualityMethodBase *create(PLCContext*, PLCInput*) = 0;
    virtual int computeQuality(PLCLine *) = 0;

    virtual const char *getName() { return this->name; }

    static QualityMethodBase *CreateQualityFunction(PLCContext *, PLCInput*,
                                                    const char *name);
};


void QualityLineCompositor(PLCContext *plContext, int line, PLCLine *lineObj);
