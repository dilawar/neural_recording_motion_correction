/*
 * =====================================================================================
 *
 *       Filename:  videoio.cpp
 *
 *    Description:  Read and write video to files.
 *
 *        Version:  1.0
 *        Created:  10/14/2016 09:22:43 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dilawar Singh (), dilawars@ncbs.res.in
 *   Organization:  NCBS Bangalore
 *
 * =====================================================================================
 */

#include "videoio.h"

#include <vector>
#include <tiffio.h>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

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
                    Mat frame ( h, w, CV_16U );

                    for ( size_t i = 0; i < h; i++ )
                        for ( size_t ii = 0; ii < w; ii++ )
                            frame.at<uint16> ( i, ii ) = ( uint16 ) ( raster[i*w+ii] );

                    frames.push_back ( frame );
                }
            }
        }
        while ( TIFFReadDirectory ( tif ) );
    }

    TIFFClose ( tif );

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

#if 0

    for ( auto f : frames )
    {
        imshow ( "frame", f );
        waitKey ( 10 );
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

    STRTOLOWER( ext );
    if ( ext == "tif" || ext == "tiff" )
        get_frames_from_tiff ( filename, frames, vidInfo );
    else 
        get_frames_from_avi ( filename, frames, vidInfo );

}

void write_frames( 
        const string& outfile                   /* Output file */
        , const vector< Mat > frames            /* All the frames */
        , const string& infile                  /* Input file. */
        )
{
    // Get the extension of file.
    size_t lastDotPos = infile.find_last_of( '.' );
    string ext = infile.substr( lastDotPos + 1 );
    if( ext.size( ) < 1 )
    {
        std::cout << "[WARN] I could not determine file type,"
            << " I am going to use avi format"
            << std::endl;
        ext = string("avi");
    }

    STRTOLOWER( ext );
    if( ext == "tiff" || ext == "tif" )
        return write_frames_to_tiff( outfile, frames, infile );

    // Now file type is determined, use opencv to get fps and fourcc codec from file.
    VideoCapture in( infile );
    double fps = in.get( CAP_PROP_FPS );        /* Get frame rate */
    if( fps < 1.0 )
    {
        std::cout << "[WARN] I could not determine frame rate from input file."
            << " I am going to use fps = 15. " << std::endl;
        fps = 15.0;
    }

    double fourcc = in.get( CAP_PROP_FOURCC );
    std::cout << "[DEBUG] Got codec " << fourcc << std::endl;
    if( fourcc == 0 )
    {
        std::cout << "[WARN] I could not determine the codec. "
            << "I am goind to default codec (MPEG)" << std::endl;
        fourcc = FOURCC_CODEC_DEFAULT;
    }
    
    in.release( );                                /* Close the input file. */


    /*-----------------------------------------------------------------------------
     *  Start writing to output file.
     *-----------------------------------------------------------------------------*/
    VideoWriter writer;
    Size frameSize ( frames[0].cols, frames[0].rows ); /* Frame size */

    writer.open ( outfile, fourcc, fps, frameSize, true );
    if ( writer.isOpened() )
    {
        for ( size_t i = 0; i < frames.size(); i ++ )
        {
            // Convert frame from greyscale to color before writing.
            Mat colorFrame;
            cvtColor( frames[i], colorFrame, CV_GRAY2BGR );
            writer << colorFrame;
        }
        writer.release( );
    }

    cout << "[INFO] Wrote " << frames.size() << " frames (grayscale) to " 
        << outfile << endl;
}


/**
 * @brief Write frames to tiff file.
 *
 * @param 
 * @param {
 */
void write_frames_to_tiff ( const string& outfile
        , const vector< Mat > frames
        , const string& infile 
        )
{
    // Open the tiff file and create tags from the infile.
    TIFF* out = TIFFOpen ( outfile.c_str(), "w" );
    TIFF* in = TIFFOpen( infile.c_str(), "r" );

    if( ! out )
    {
        std::cout << "Can't open tiff file to open : " << outfile << std::endl;
        return;
    }

    if( ! in )
    {
        std::cout << "Can't open " << infile << " for reading tags" << std::endl;
        return;
    }

    uint32 height, width;
    tsize_t sampleperpixel;
    tsize_t linebytes;
    uint16 spp, bpp, photo, resUnit;

    for (uint16 frameNum = 0; frameNum < frames.size(); frameNum++) 
    {
        TIFFSetField( out, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
        TIFFSetField( out, TIFFTAG_PAGENUMBER, frameNum, frames.size( ) );

        TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &width);
        TIFFSetField ( out, TIFFTAG_IMAGEWIDTH, width );

        TIFFGetField( in, TIFFTAG_IMAGELENGTH, &height);
        TIFFSetField ( out, TIFFTAG_IMAGELENGTH, height);

        TIFFGetField( in, TIFFTAG_SAMPLESPERPIXEL, &spp );
        TIFFSetField ( out, TIFFTAG_SAMPLESPERPIXEL, spp);

        TIFFGetField( in, TIFFTAG_BITSPERSAMPLE, &bpp );
        TIFFSetField ( out, TIFFTAG_BITSPERSAMPLE, 8 );

        TIFFSetField ( out, TIFFTAG_ORIENTATION, ORIENTATION_BOTLEFT ); 
        TIFFSetField ( out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG );
        TIFFSetField ( out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK );

        Mat frame = frames[frameNum];
        // We set the strip size of the file to be size of one row of pixels
        //TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, width*spp));

        // Data type matching.
        unsigned char* row = frame.data;
        // See http://opencv-users.1802565.n2.nabble.com/Mat-class-with-unsigned-int-type-or-long-int-type-td7197221.html
        for( uint16 c = 0; c < height; c ++ )
            TIFFWriteScanline( out, &row[c*width], c, 0);
            
        TIFFWriteDirectory( out );
    }

    // Close the infile.
    TIFFClose( in );
    TIFFClose( out );
    std::cout << "[INFO] Wrote corrected frames to " << outfile << std::endl;
}


