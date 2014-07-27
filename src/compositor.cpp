/**
 * Purpose: Pixel Lapse Compositor mainline
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

#include <wjreader.h>

#include "compositor.h"
#include "cpl_progress.h"

/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

void Usage()

{
    printf( "Usage: compositor --help --help-general\n" );
    printf( "         -o output_file [-st source_trace_file] [-qo quality]\n" );
    printf( "         [-s name value]* [-q] [-v] [-dp pixel line]\n" );
    printf( "         [-i input_file [-c cloudmask] [-qm name value]*]*\n" );
    exit(1);
}

/************************************************************************/
/*                              LoadJSON()                              */
/************************************************************************/

static void LoadJSON(PLCContext &plContext, const char *json_filename)

{
/* -------------------------------------------------------------------- */
/*      Load and parse JSON.                                            */
/* -------------------------------------------------------------------- */
    FILE *fp = fopen(json_filename, "r");
    if( fp == NULL )
    {
        CPLError(CE_Fatal, CPLE_AppDefined,
                 "Failed to open %s: %s", 
                 json_filename, strerror(errno));
    }

    WJReader readjson;
    WJElement json;

    if(!(readjson = WJROpenFILEDocument(fp, NULL, 0)) ||
       !(json = WJEOpenDocument(readjson, NULL, NULL, NULL))) {
        fprintf(stderr, "json could not be read.\n");
        exit(3);
    }

/* -------------------------------------------------------------------- */
/*      Configure context from json.                                    */
/* -------------------------------------------------------------------- */
    plContext.initializeFromJson(json);

    WJECloseDocument(json);
    WJRCloseDocument(readjson);
    fclose(fp);
}

/************************************************************************/
/*                                main()                                */
/************************************************************************/

int main(int argc, char **argv)

{
/* -------------------------------------------------------------------- */
/*      Generic GDAL startup.                                           */
/* -------------------------------------------------------------------- */
    GDALAllRegister();
    argc = GDALGeneralCmdLineProcessor( argc, &argv, 0 );
    if( argc < 1 )
        exit( -argc );

/* -------------------------------------------------------------------- */
/*      Handle command line arguments.                                  */
/* -------------------------------------------------------------------- */
    PLCContext plContext;
    GDALProgressFunc pfnProgress = GDALTermProgress;

    for( int i = 1; i < argc; i++ )
    {
        if( EQUAL(argv[i],"--help") || EQUAL(argv[i],"-h"))
            Usage();

        else if( EQUAL(argv[i],"-i") )
        {
            // Consume a whole PLCInput definition.
            PLCInput *input = new PLCInput(plContext.inputFiles.size());
            int argsConsumed = input->ConsumeArgs(argc-i, argv+i);
            i += argsConsumed-1;
            plContext.inputFiles.push_back(input);
        }

        else if( EQUAL(argv[i],"-j") && i < argc-1 )
        {
            // Consume and process JSON definition.
            LoadJSON(plContext, argv[i+1]);
            i += 1;
        }

        else if( EQUAL(argv[i],"-s") && i < argc-2 )
        {
            plContext.strategyParams.AddNameValue(argv[i+1], argv[i+2]);
            i += 2;
        }

        else if( EQUAL(argv[i],"-o") && i < argc-1 
                 && EQUAL(plContext.outputFilename,""))
        {
            plContext.outputFilename = argv[++i];
        }

        else if( EQUAL(argv[i],"-st") && i < argc-1 
                 && EQUAL(plContext.sourceTraceFilename,""))
        {
            plContext.sourceTraceFilename = argv[++i];
        }

        else if( EQUAL(argv[i],"-qo") && i < argc-1 
                 && EQUAL(plContext.qualityFilename,""))
        {
            plContext.qualityFilename = argv[++i];
        }

        else if( EQUAL(argv[i],"-q") )
        {
            plContext.quiet = TRUE;
            pfnProgress = GDALDummyProgress;
        }

        else if( EQUAL(argv[i],"-dp") && i < argc-2 )
        {
            plContext.debugPixels.push_back(atoi(argv[i+1]));
            plContext.debugPixels.push_back(atoi(argv[i+2]));
            i += 2;
        }

        else if( EQUAL(argv[i],"-v") )
        {
            plContext.verbose++;
        }

        else
        {
            fprintf(stderr, "Unexpected argument:%s\n", argv[i]);
            Usage();						
        }
    }

    if( plContext.inputFiles.size() == 0 
        || plContext.outputFilename.size() == 0)
        Usage();

/* -------------------------------------------------------------------- */
/*      In some contexts, it is easier to pass debug requests via       */
/*      environment variable.                                           */
/* -------------------------------------------------------------------- */
    if( getenv("DEBUG_PIXELS") != NULL )
    {
        CPLStringList values(CSLTokenizeStringComplex(
                                 getenv("DEBUG_PIXELS"), ",",
                                 FALSE, FALSE));
        for( int i = 0; i < values.size(); i++ )
            plContext.debugPixels.push_back(atoi(values[i]));
    }

/* -------------------------------------------------------------------- */
/*      Confirm that all inputs and outputs are the same size.  We      */
/*      will assume they are in a consistent coordinate system.         */
/* -------------------------------------------------------------------- */
    plContext.outputDS = (GDALDataset *) 
        GDALOpen(plContext.outputFilename, GA_Update);
    if( plContext.outputDS == NULL )
        exit(1);

    plContext.width = plContext.outputDS->GetRasterXSize();
    plContext.height = plContext.outputDS->GetRasterYSize();

    for( unsigned int i=0; i < plContext.inputFiles.size(); i++ )
    {
        plContext.inputFiles[i]->Initialize(&plContext);

        GDALDataset *inputDS = plContext.inputFiles[i]->getDS();
        if( inputDS->GetRasterXSize() != plContext.width
            || inputDS->GetRasterYSize() != plContext.height)
        {
            CPLError(CE_Fatal, CPLE_AppDefined,
                     "Size of %s (%dx%d) does not match target %s (%dx%d)",
                     plContext.inputFiles[i]->getFilename(),
                     inputDS->GetRasterXSize(),
                     inputDS->GetRasterYSize(),
                     plContext.outputFilename.c_str(),
                     plContext.outputDS->GetRasterXSize(),
                     plContext.outputDS->GetRasterYSize());
        }
    }

/* -------------------------------------------------------------------- */
/*      Initialize the quality methods.                                 */
/* -------------------------------------------------------------------- */
    if( plContext.qualityMethods.size() == 0 )
        plContext.initializeQualityMethods(NULL);

/* -------------------------------------------------------------------- */
/*      Create source trace file if requested.                          */
/* -------------------------------------------------------------------- */
    if( !EQUAL(plContext.sourceTraceFilename,"") )
    {
        GDALDriver *tiffDriver = (GDALDriver *) GDALGetDriverByName("GTiff");
        CPLStringList createOptions;
        GDALDataType stPixelType = GDT_Byte;
        if( plContext.inputFiles.size() > 255 )
            stPixelType = GDT_UInt16;

        createOptions.AddString("COMPRESS=LZW");
        plContext.sourceTraceDS = 
            tiffDriver->Create(plContext.sourceTraceFilename,
                               plContext.width, plContext.height, 1,
                               stPixelType, createOptions);
        plContext.sourceTraceDS->SetProjection(
            plContext.outputDS->GetProjectionRef());
        
        double geotransform[6];
        plContext.outputDS->GetGeoTransform(geotransform);
        plContext.sourceTraceDS->SetGeoTransform(geotransform);

        CPLStringList sourceMD;
        CPLString key;

        for( unsigned int i=0; i < plContext.inputFiles.size(); i++ )
        {
            key.Printf("SOURCE_%d", i+1);
            sourceMD.SetNameValue(key, plContext.inputFiles[i]->getFilename());
        }

        plContext.sourceTraceDS->SetMetadata(sourceMD);
    }

/* -------------------------------------------------------------------- */
/*      Create quality file if requested.                               */
/* -------------------------------------------------------------------- */
    if( !EQUAL(plContext.qualityFilename,"") )
    {
        GDALDriver *tiffDriver = (GDALDriver *) GDALGetDriverByName("GTiff");
        plContext.qualityDS = 
            tiffDriver->Create(plContext.qualityFilename,
                               plContext.width, plContext.height,
                               plContext.inputFiles.size() + 1,
                               GDT_Float32, NULL);
        plContext.qualityDS->SetProjection(
            plContext.outputDS->GetProjectionRef());
        
        double geotransform[6];
        plContext.outputDS->GetGeoTransform(geotransform);
        plContext.qualityDS->SetGeoTransform(geotransform);

        for( unsigned int i=0; i < plContext.inputFiles.size(); i++ )
        {
            plContext.qualityDS->GetRasterBand(i+2)->
                SetDescription(plContext.inputFiles[i]->getFilename());
        }

        plContext.qualityDS->GetRasterBand(1)->
            SetDescription(plContext.outputFilename);
    }

/* -------------------------------------------------------------------- */
/*      Run through the image processing scanlines.                     */
/* -------------------------------------------------------------------- */
    for(int line=0; line < plContext.outputDS->GetRasterYSize(); line++ )
    {
        pfnProgress(line / (double) plContext.height, NULL, NULL);

        plContext.line = line;
        PLCLine *lineObj = plContext.getOutputLine(line);

        LineCompositor(&plContext, line, lineObj );

        plContext.writeOutputLine(line, lineObj);
        
        delete lineObj;
    }
    pfnProgress(1.0, NULL, NULL);

    GDALClose(plContext.outputDS);

    if( plContext.sourceTraceDS )
        GDALClose(plContext.sourceTraceDS);
    if( plContext.qualityDS )
        GDALClose(plContext.qualityDS);

/* -------------------------------------------------------------------- */
/*      Reporting?                                                      */
/* -------------------------------------------------------------------- */
    if( plContext.verbose )
    {
        plContext.qualityHistogram.report(stdout, "final_quality");
    }

    exit(0);
}
