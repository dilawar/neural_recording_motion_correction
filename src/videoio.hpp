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
//
#include "tinytiffreader.h"
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

typedef struct VideoInfo 
{
    size_t width = 0;
    size_t height = 0;
    float fps = 0;
    size_t numFrames = 0;
} video_info_t;

#if 0
/**
 * @brief Convert a 32bit pixal value from TIFF to 8 bit gray code.
 *
 * @param val 32 bit = 4 time 8 bit values (R, G, B, A).
 *
 * @return 
 */
template< typename T>
T toGray( uint32 val, size_t bitPerSample )
{
    // Slice R, G, B, and A values.
    if( bitPerSample == 4 )
    {
        T R, G, B;
        R = ( val >> 0 ) & 0xFF;
        G = ( val >> 8 ) & 0xFF;
        B = ( val >> 16 ) & 0xFF;
        //printf( "val: %ld, A = %d, B = %d, G = %d, R = %d\n", val, A, B, G, R );
        //exit( 1 );
        //unsigned int pixal = ceil( 0.2126 * R + 0.7152 * G + 0.0722 * B);
        //return (uint8) pixal;
        //return ( (R << 1) + (G << 1) + (B << 1) );
        return R + G + B;
    }
    else if( bitPerSample == 8 )
    {
        T x, y;
        x = (val >> 0 ) & 0xFFFF;
        y = (val >> bitPerSample ) && 0xFFFF;
        return x + y;
    }
}
#endif

/**
 * @brief  Read data from TIFF images are vector of opencv matrix.
 *
 * @tparam pixal_type_t
 * @param 
 * @param 
 * @param 
 */
template< typename pixal_type_t >
void get_frames_from_tiff( const string& filename
        , vector< Mat_< pixal_type_t > >& frames 
        , video_info_t& vidInfo
        )
{
    TinyTIFFReaderFile* tif = TinyTIFFReader_open( filename.c_str() );
    if (tif) {
        do {
            uint32_t width = TinyTIFFReader_getWidth( tif );
            uint32_t height = TinyTIFFReader_getHeight( tif );
            uint16_t* data = (uint16_t*) calloc( width*height, sizeof( uint16_t ) );
            TinyTIFFReader_getSampleData( tif, data, 0 );
            Mat frame(width, height, CV_16U, data);
            frame.convertTo( frame, CV_8U, 1/8.0 );
            //cout << frame << endl;
            frames.push_back( frame );
            free( data );
        } while ( TinyTIFFReader_readNext( tif ) );
    }
    TinyTIFFReader_close( tif );
}

template< typename pixal_type_t >
void get_frames_from_avi( const string& filename
        , vector< Mat_< pixal_type_t > >& frames 
        , video_info_t& vidInfo
        )
{
    VideoCapture inputVideo( filename.c_str() );

    if(! inputVideo.isOpened())
    {
        std::cout << "Could not open " << filename << std::endl;
        return;
    }


    vidInfo.width = (int) inputVideo.get(CV_CAP_PROP_FRAME_WIDTH);
    vidInfo.height = (int) inputVideo.get(CV_CAP_PROP_FRAME_HEIGHT);

    while(true) 
    {
        Mat cur, curGrey;
        inputVideo >> cur;
        if(cur.data == NULL) {
            break;
        }
        cvtColor(cur, curGrey, COLOR_BGR2GRAY);
        frames.push_back( curGrey );
        vidInfo.numFrames += 1;
    }

    inputVideo.release( );
    cout << "[INFO] Read " << frames.size() << " frames from "
        << filename << endl;
}

template< typename pixal_type_t >
void read_frames( const string& filename
        , vector< Mat_<pixal_type_t> >& frames 
        , video_info_t& vidInfo
        )
{

    string::size_type pAt = filename.find_last_of('.');       
    string ext = filename.substr( pAt+1 );
    std::cout << "[INFO] Extenstion of file " << ext << std::endl;

    if( ext == "tif" || ext == "tiff" )
    {
        std::cout << "[INFO] Got a tiff file" << std::endl;
        get_frames_from_tiff<pixal_type_t>( filename, frames, vidInfo );
    }
    else if( ext == "avi" )
    {
        std::cout << "[INFO] Got a avi file" << std::endl;
        get_frames_from_avi<pixal_type_t>( filename, frames, vidInfo );
    }

}


template< typename pixal_type_t>
void write_frames_to_avi( const string& avifile
        , vector< Mat_<pixal_type_t> > frames 
        , double fps
        )
{

    VideoWriter writer; 

    /*-----------------------------------------------------------------------------
     * Write corrected video to a avi file.
     *-----------------------------------------------------------------------------*/
    Size frameSize( frames[0].cols, frames[0].rows );
    writer.open( avifile, FOURCC_CODEC, fps, frameSize, true);
    if( writer.isOpened() )
    {
        for( size_t i = 0; i < frames.size(); i ++ )
        {
            Mat colorFrame;
            cvtColor( frames[i], colorFrame, CV_GRAY2BGR );
            writer << colorFrame;
        }
        writer.release( );
    }
    cout << "Wrote " << frames.size() << " frames to " << avifile << endl;
}

#endif   /* ----- #ifndef videoio_INC  ----- */
