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

#include <map>
#include "gdal_priv.h"

#include <wjelement.h>

class QualityMethodBase;
class PLCContext;

////////////////////////////////////////////////////////////////////////////
class PLCLine {
    int     width;
    
    int     bandCount;
    std::vector<short*> bandData;
    unsigned short  *cloud;
    GByte  *alpha;
    float  *quality;
    float  *newQuality;
    unsigned short  *source;
    
  public:
    PLCLine(int width);
    virtual ~PLCLine();

    int     getWidth() { return width; }
    int     getBandCount() { return bandCount; }
    short  *getBand(int);
    GByte  *getAlpha();
    unsigned short  *getCloud();
    unsigned short  *getSource();
    float  *getQuality();
    float  *getNewQuality();
};

////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////
class PLCInput {
    CPLString    filename;
    GDALDataset *DS;
    
    CPLString    cloudMask;
    GDALDataset *cloudDS;
    
    std::map <CPLString,double> qualityMetrics;
    std::map <CPLString,CPLString> parameters;

    PLCHistogram qualityHistogram;
    PLCHistogram cloudQualityHistogram;

    int          inputIndex;
    
  public:
                 PLCInput(int inputIndex = -1);
    virtual     ~PLCInput();

    int          ConsumeArgs(int argc, char **argv);
    void         ConsumeJson(WJElement);
    void         Initialize(PLCContext *);

    double       getQM(const char *key, double defaultValue = -1.0);
    const char  *getParm(const char *key, const char *defaultValue = NULL);

    const char  *getFilename() { return filename; }
    GDALDataset *getDS();

    const char  *getCloudFilename() { return cloudMask; }
    GDALDataset *getCloudDS();

    PLCLine     *getLine(int line);

    int          getInputIndex() { return inputIndex; }
};

////////////////////////////////////////////////////////////////////////////
class PLCContext {
  public:
    PLCContext();
    virtual ~PLCContext();

    void          initializeFromJson(WJElement);
    void          initializeQualityMethods(WJElement);
    int           width;
    int           height;
    int           line;
    
    int           quiet;
    int           verbose;
    int           currentLine;

    std::vector<int> debugPixels;
    int           isDebugPixel(int pixel, int line);
    int           isDebugLine(int line);
    
    CPLStringList strategyParams;
    const char   *getStratParam(const char *name, const char *def=NULL) {
        return strategyParams.FetchNameValueDef(name, def);
    }

    CPLString     outputFilename;
    GDALDataset  *outputDS;

    CPLString     sourceTraceFilename;
    GDALDataset  *sourceTraceDS;

    CPLString     qualityFilename;
    GDALDataset  *qualityDS;

    std::vector<PLCInput*> inputFiles;
    std::vector<QualityMethodBase*> qualityMethods;

    PLCLine *     getOutputLine(int line);
    void          writeOutputLine(int line, PLCLine *);

    PLCHistogram  qualityHistogram;
};

////////////////////////////////////////////////////////////////////////////
class QualityMethodBase {
  protected:
    QualityMethodBase(const char *name);
    CPLString name;

  public:
    virtual ~QualityMethodBase();

    virtual QualityMethodBase *create(PLCContext*, WJElement node) = 0;

    virtual int computeQuality(PLCInput *, PLCLine *) = 0;
    virtual int computeStackQuality(PLCContext *, std::vector<PLCLine *>&);
    //virtual int computeStackQuality(PLContext *, std::vector<PLCLine *>&);

    virtual void mergeQuality(PLCInput *, PLCLine *);

    virtual const char *getName() { return this->name; }

    static QualityMethodBase *CreateQualityFunction(PLCContext *,
                                                    WJElement node,
                                                    const char *name);
};

void LineCompositor(PLCContext *plContext, int line, PLCLine *lineObj);
