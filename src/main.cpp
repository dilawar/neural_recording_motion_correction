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


using namespace std;
using namespace cv;

int main(int argc, char **argv)
{
    if(argc < 3) {
        cout << "./videostab input_file output_file" << endl;
        return 0;
    }



    // Step 0. Prepare output file.
    string infile( argv[1] );                   /* Input file */

    string::size_type pAt = infile.find_last_of('.');       
    string ext = infile.substr( pAt+1 );
    std::cout << "[INFO] Extenstion of file " << ext << std::endl;

    video_info_t vInfo;
    vector< Mat_<unsigned char> > frames; 
    read_frames( infile, frames, vInfo );

    vector< Mat_<unsigned char> > stablizedFrames;
    stabilize( frames, stablizedFrames );
    std::cout << "Corrected frames " << stablizedFrames.size() << std::endl;


    /*-----------------------------------------------------------------------------
     *  Write results to files.
     *-----------------------------------------------------------------------------*/

    /*-----------------------------------------------------------------------------
     * Write corrected video to a avi file.
     *-----------------------------------------------------------------------------*/
    double fps = 15.0;
    write_frames_to_avi( string(argv[2]), frames, fps );


    /*-----------------------------------------------------------------------------
     *  Write corrected video and non-corrected video to combined.
     *-----------------------------------------------------------------------------*/
    vector< Mat_<unsigned char> > combinedFrames;
    string combinedVideofileName = "__combined.avi";
    for (size_t i = 0; i < stablizedFrames.size( ); i++) 
    {
        Mat combined;
        hconcat( stablizedFrames[i], frames[i], combined );
        combinedFrames.push_back( combined );
    }
    write_frames_to_avi( combinedVideofileName, combinedFrames, fps );

    return 0;
}
