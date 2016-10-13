/*
Copyright (c) 2014, Nghia Ho
Copyright (c) 2016 -, Dilawar Singh  <dilawars@ncbs.res.in>

All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

--------------------------------------------------------------------------------
Modification log:

    Wed 12 Oct 2016 10:59:23 AM IST , Dilawar Singh

        - Compiled with opencv-3 on openSUSE-Tumbleweed.
        - Added CMake support.
        - Added tiff support.
        - 
*/
 
#include <opencv2/opencv.hpp>
#include <iostream>
#include <cassert>
#include <cmath>
#include <fstream>
#include "videoio.hpp"
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
                , "num-passes" , "Number of passes to perform to stabilize video."
                " (default 2). Increase this number for better result."
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
        outfile = infile + "_corrected.avi";

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
     * Write corrected video to a avi file.
     *-----------------------------------------------------------------------------*/
    double fps = 15.0;
    write_frames_to_avi( outfile, stablizedFrames, fps );

    /*-----------------------------------------------------------------------------
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
    write_frames_to_avi( combinedVideofileName, combinedFrames, fps );

    return 0;
}
