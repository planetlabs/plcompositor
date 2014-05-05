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

#include "compositor.h"
#include "cpl_progress.h"

/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

void Usage()

{
    printf( "Usage: compositor --help --help-general\n" );
    printf( "         -o output_file\n" );
    printf( "         [-s name value]* [-q] [-v]\n" );
    printf( "         [-i input_file [-c cloudmask] [-qm name value]*]*\n" );
    exit(1);
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
            PLCInput *input = new PLCInput();
            int argsConsumed = input->ConsumeArgs(argc-i, argv+i);
            i += argsConsumed-1;
            plContext.inputFiles.push_back(input);
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

        else if( EQUAL(argv[i],"-q") )
        {
            plContext.quiet = TRUE;
            pfnProgress = GDALDummyProgress;
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
/*      Confirm that all inputs and outputs are the same size.  We      */
/*      will assume they are in a consistent coordinate system.         */
/* -------------------------------------------------------------------- */
    plContext.outputDS = (GDALDataset *) 
        GDALOpen(plContext.outputFilename, GA_Update);
    if( plContext.outputDS == NULL )
        exit(1);

    for( unsigned int i=0; i < plContext.inputFiles.size(); i++ )
    {
        plContext.inputFiles[i]->Initialize(&plContext);

        GDALDataset *inputDS = plContext.inputFiles[i]->getDS();
        if( inputDS->GetRasterXSize() != plContext.outputDS->GetRasterXSize()
            || inputDS->GetRasterYSize() != plContext.outputDS->GetRasterYSize())
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
/*      Run through the image processing scanlines.                     */
/* -------------------------------------------------------------------- */
    for(int line=0; line < plContext.outputDS->GetRasterYSize(); line++ )
    {
        pfnProgress(line / (double) plContext.outputDS->GetRasterYSize(),
                    NULL, NULL);

        PLCLine *lineObj = plContext.getOutputLine(line);

        QualityLineCompositor(&plContext, line, lineObj );

        plContext.writeOutputLine(line, lineObj);
        
        delete lineObj;
    }
    pfnProgress(1.0, NULL, NULL);

    GDALClose(plContext.outputDS);

/* -------------------------------------------------------------------- */
/*      Reporting?                                                      */
/* -------------------------------------------------------------------- */
    if( plContext.verbose )
    {
        plContext.qualityHistogram.report(stdout, "final_quality");
    }

    exit(0);
}
