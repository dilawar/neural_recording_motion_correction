/***
 *       Filename:  main.cpp
 *
 *    Description:  description
 *
 *        Version:  0.0.1
 *        Created:  2016-10-13
 *       Revision:  none
 *
 *         Author:  Dilawar Singh <dilawars@ncbs.res.in>
 *   Organization:  NCBS Bangalore
 *
 *        License:  GNU GPL2
 *
 *   NOTE :
 *
 *   This application is built over code posted by Nghia Ho on his blog
 *   http://nghiaho.com/?p=2093 (accessed Oct 13, 2016). 
 **/

 
#include <opencv2/opencv.hpp>
#include <iostream>
#include <cassert>
#include <cmath>
#include <fstream>
#include "videoio.h"
#include "motion_stabilizer.hpp"
#include "tclap/CmdLine.h"

using namespace std;
using namespace cv;

static bool verbose_flag_ = false;

int main(int argc, char **argv)
{
    /*-----------------------------------------------------------------------------
     *  Command line options.
     *-----------------------------------------------------------------------------*/
    string infile;
    string outfile;
    size_t numPasses = 1;

    try {  

        TCLAP::CmdLine cmd("Utility to stabilize video.", ' ', "0.1.0");
        TCLAP::ValueArg<std::string> inputArg("i"
                ,"input" ,"Input file (tif or avi)"
                ,true ,"" ,"file path"
                );
        cmd.add( inputArg );

        TCLAP::ValueArg<std::string> outputArg("o"
                , "output" , "path to save corrected file (tif or avi)"
                , false ,"" ,"file path"
                );
        cmd.add( outputArg );

        TCLAP::ValueArg<size_t> numpassArg ("n"
                , "num-passes" 
                , "Number of passes to perform to stabilize video."
                " (default 2). Increase this number for better result.\n"
                " On normal recording, 1 would be sufficient but on recording "
                " where features are small, you should increase it "
                , false , 2 , "postitive integer"
                );
        cmd.add( numpassArg );

        TCLAP::SwitchArg verbose("v", "verbose", "Make output verbose", cmd, false);

        cmd.parse( argc, argv );

        infile = inputArg.getValue();
        outfile = outputArg.getValue( );
        numPasses = numpassArg.getValue( );
        verbose_flag_ = verbose.getValue( );

    } 
    catch (TCLAP::ArgException &e)
    { 
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl; 
    }

    /*-----------------------------------------------------------------------------
     *  All right, command line options are parsed. Make sure when output file
     *  name is not set we set a default output file name.
     *-----------------------------------------------------------------------------*/

    string::size_type pAt = infile.find_last_of('.');       
    string ext = infile.substr( pAt+1 );
    if( outfile.size() < 1 )
        outfile = infile + "_corrected." + ext;

    std::cout << "[DEBUG] In file " << infile  << std::endl;
    std::cout << "[DEBUG] Out file" << outfile << std::endl;

    video_info_t vInfo;
    vector< Mat > frames; 
    read_frames( infile, frames, vInfo );

    /*-----------------------------------------------------------------------------
     *  Some time multiple passes are neccessary to correct the data.
     *-----------------------------------------------------------------------------*/
    auto initFrames = frames;
    vector< Mat > stablizedFrames;
    for (size_t i = 0; i < numPasses - 1 ; i++) 
    {
        std::cout << "[INFO] Running pass " << i + 1 <<  " out of " << numPasses 
            << std::endl;
        stabilize( initFrames, stablizedFrames );
        initFrames = stablizedFrames;
        stablizedFrames.clear( );
    }

    std::cout << "[INFO] Final pass " << endl;
    stabilize( initFrames, stablizedFrames );
    std::cout << "Corrected frames " << stablizedFrames.size() << std::endl;

    /*-----------------------------------------------------------------------------
     * Write corrected video to output file. Use the format, fps and codec
     * similar to input file. 
     * 
     * FIXME: Currently output is only gray-scale.
     *-----------------------------------------------------------------------------*/
    write_frames( outfile, stablizedFrames, infile);

    if( verbose_flag_ )
    {
        /*-----------------------------------------------------------------------------
         *  Optional:
         *
         *  Write corrected video and non-corrected video to combined.
         *-----------------------------------------------------------------------------*/
        vector< Mat > combinedFrames;
        string combinedVideofileName = "__combined.avi";
        for (size_t i = 0; i < stablizedFrames.size( ); i++) 
        {
            Mat combined;
            hconcat( frames[i], stablizedFrames[i], combined );
            combinedFrames.push_back( combined );
        }
        write_frames( combinedVideofileName, combinedFrames, infile );
    }

    return 0;
}
