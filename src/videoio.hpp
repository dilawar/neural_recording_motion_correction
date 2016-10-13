/*
 * =====================================================================================
 *
 *       Filename:  videoio.hpp
 *
 *    Description:  Read video files into opencv format.
 *
 *        Version:  1.0
 *        Created:  10/12/2016 01:25:38 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dilawar Singh (), dilawars@ncbs.res.in
 *   Organization:  NCBS Bangalore
 *
 * =====================================================================================
 */


#ifndef  videoio_INC
#define  videoio_INC

//#define FOURCC_CODEC CV_FOURCC( 'D', 'I', 'V', 'X' )
#define FOURCC_CODEC CV_FOURCC( 'M', 'J', 'P', 'G' )
//#define FOURCC_CODEC CV_FOURCC( 'L', 'A', 'G', 'S' )

#include <opencv2/opencv.hpp>

#ifdef USE_LIBTIFF 
#include <tiffio.h>
#elif USE_TINYTIFF 
#include "tinytiffreader.h"
#else

#endif

using namespace std;
using namespace cv;

typedef struct VideoInfo
{
    size_t width = 0;
    size_t height = 0;
    float fps = 0;
    size_t numFrames = 0;
} video_info_t;

/**
 * @brief  Read data from TIFF images are vector of opencv matrix.
 *
 * @tparam pixal_type_t
 * @param
 * @param
 * @param
 */
void get_frames_from_tiff ( const string& filename
                            , vector< Mat > & frames
                            , video_info_t& vidInfo
                          )
{

#ifdef USE_LIBTIFF
    TIFF *tif = TIFFOpen ( filename.c_str(), "r" );

    if ( tif )
    {
        int dircount = 0;

        do
        {
            dircount++;
            uint32 w, h;
            short bitsPerSample;
            size_t npixals;

            TIFFGetField ( tif, TIFFTAG_IMAGEWIDTH, &w );
            TIFFGetField ( tif, TIFFTAG_IMAGELENGTH, &h );
            TIFFGetField ( tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample );
            //std::cout << "Bits per sample " << bitsPerSample  << std::endl;
            npixals = w * h;

            //std::cout << "Height : " << h << std::endl;
            //std::cout << "Width : " << w << std::endl;

            uint32* raster = ( uint32* ) _TIFFmalloc ( npixals * sizeof ( uint32 ) );

            if ( raster != NULL )
            {
                if ( TIFFReadRGBAImage ( tif, w, h, raster, 0 )  )
                {
                    Mat frame ( w, h, CV_16U );
                    for (size_t i = 0; i < h; i++) 
                        for (size_t ii = 0; ii < w; ii++) 
                            frame.at<uint16>(i, ii) = (uint16)( raster[i*w+ii] & 0x0000FFFF );
                    frames.push_back ( frame );
                }
            }
        }
        while ( TIFFReadDirectory ( tif ) );
    }
    TIFFClose ( tif );

#elif USE_TINYTIFF

    TinyTIFFReaderFile* tiffr = NULL;
    tiffr = TinyTIFFReader_open ( filename.c_str() );

    if ( !tiffr )
    {
        std::cerr <<
                  "[ERROR] reading (not existent, not accessible or no TIFF file)"
                  << endl;
    }
    else
    {
        if ( TinyTIFFReader_wasError ( tiffr ) )
            std::cerr << "[ERR] " << TinyTIFFReader_getLastError ( tiffr ) << endl;

        uint32_t nframes = TinyTIFFReader_countFrames ( tiffr );

        if ( TinyTIFFReader_wasError ( tiffr ) )
            std::cout << "[ERROR] " << TinyTIFFReader_getLastError ( tiffr ) << endl;

        uint32_t totalReadFrames = 0;

        do
        {
            uint32_t width = TinyTIFFReader_getWidth ( tiffr );
            uint32_t height = TinyTIFFReader_getHeight ( tiffr );
            bool ok = true;

            if ( width > 0 && height > 0 ) 
            {
                totalReadFrames++;
                uint16_t* image = ( uint16_t* ) calloc ( width * height, sizeof ( uint16_t ) );
                TinyTIFFReader_getSampleData ( tiffr, image, 0 );

                if ( TinyTIFFReader_wasError ( tiffr ) )
                {
                    std::cout << "[ERROR] " << TinyTIFFReader_getLastError ( tiffr )
                              << endl;
                    continue;
                }

                Mat grey ( width, height, CV_16U, image );
                frames.push_back ( grey );
                free ( image );
            }
        }
        while ( TinyTIFFReader_readNext ( tiffr ) ); // iterate over all frames

        std::cout << " [INFO] Read " << totalReadFrames << " frames" << endl;
    }

    TinyTIFFReader_close ( tiffr );

#else
    imreadmulti ( String ( filename.c_str() )
                  , frames
                  , IMREAD_ANYDEPTH
                );
#endif


    /*-----------------------------------------------------------------------------
     *  Normalize each frame such that values are between 0 and 255.
     *-----------------------------------------------------------------------------*/
    double maxPixalValue = 0;
    double minVal , maxVal;
    Point minLoc, maxLoc;

    for ( size_t i = 0; i < frames.size(); i++ )
    {
        minMaxLoc ( frames[i], &minVal, &maxVal, &minLoc, &maxLoc );

        if ( maxVal > maxPixalValue )
            maxPixalValue = maxVal;
    }

    // Now rescale every frame to get the value between 0 and 255.
    if ( maxPixalValue > 255.0 )
    {
        for ( size_t i = 0; i < frames.size(); i++ )
        {
            Mat frame = frames[i];
            frame.convertTo ( frame, CV_8U, 255.0 / maxPixalValue );
            frames[i] = frame;
        }
    }

#if 1 
    for(auto f : frames )
    {
        imshow( "frame", f );
        waitKey( 10 );
    }
#endif 

}

void get_frames_from_avi ( const string& filename
                           , vector< Mat >& frames
                           , video_info_t& vidInfo
                         )
{
    VideoCapture inputVideo ( filename.c_str() );

    if ( ! inputVideo.isOpened() )
    {
        std::cout << "Could not open " << filename << std::endl;
        return;
    }


    vidInfo.width = ( int ) inputVideo.get ( CV_CAP_PROP_FRAME_WIDTH );
    vidInfo.height = ( int ) inputVideo.get ( CV_CAP_PROP_FRAME_HEIGHT );

    while ( true )
    {
        Mat cur, curGrey;
        inputVideo >> cur;

        if ( cur.data == NULL )
        {
            break;
        }

        cvtColor ( cur, curGrey, COLOR_BGR2GRAY );
        frames.push_back ( curGrey );
        vidInfo.numFrames += 1;
    }

    inputVideo.release( );
    cout << "[INFO] Read " << frames.size() << " frames from "
         << filename << endl;
}

void read_frames ( const string& filename
                   , vector< Mat >& frames
                   , video_info_t& vidInfo
                 )
{

    string::size_type pAt = filename.find_last_of ( '.' );
    string ext = filename.substr ( pAt + 1 );
    std::cout << "[INFO] Extenstion of file " << ext << std::endl;

    if ( ext == "tif" || ext == "tiff" )
    {
        std::cout << "[INFO] Got a tiff file" << std::endl;
        get_frames_from_tiff ( filename, frames, vidInfo );
    }
    else if ( ext == "avi" )
    {
        std::cout << "[INFO] Got a avi file" << std::endl;
        get_frames_from_avi ( filename, frames, vidInfo );
    }

}

void write_frames_to_avi ( const string& avifile
                           , vector< Mat > frames
                           , double fps
                         )
{

    VideoWriter writer;

    /*-----------------------------------------------------------------------------
     * Write corrected video to a avi file.
     *-----------------------------------------------------------------------------*/
    Size frameSize ( frames[0].cols, frames[0].rows );
    writer.open ( avifile, FOURCC_CODEC, fps, frameSize, true );

    if ( writer.isOpened() )
    {
        for ( size_t i = 0; i < frames.size(); i ++ )
        {
            Mat colorFrame;
            cvtColor ( frames[i], colorFrame, CV_GRAY2BGR );
            writer << colorFrame;
        }

        writer.release( );
    }

    cout << "Wrote " << frames.size() << " frames to " << avifile << endl;
}

#endif   /* ----- #ifndef videoio_INC  ----- */
