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
#include <tiffio.h>
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

/**
 * @brief Convert a 32bit pixal value from TIFF to 8 bit gray code.
 *
 * @param val 32 bit = 4 time 8 bit values (R, G, B, A).
 *
 * @return 
 */
uint16  toGray( uint32 val )
{
    // Drop last 8 bits. They are useless.
    val = val & 0x0000FFFF;
    return (uint16)val;
}

/**
 * @brief  Read data from TIFF images are vector of opencv matrix.
 *
 * @tparam pixal_type_t
 * @param 
 * @param 
 * @param 
 */
void get_frames_from_tiff( const string& filename
        , vector< Mat > & frames 
        , video_info_t& vidInfo
        )
{

#ifdef USE_LIBTIFF
    TIFF *tif = TIFFOpen( filename.c_str(), "r");
    if (tif) 
    {
        int dircount = 0;
        do 
        {
            dircount++;
            uint32 w, h;
            short bitsPerSample;
            size_t npixals;

            TIFFGetField( tif, TIFFTAG_IMAGEWIDTH, &w);
            TIFFGetField( tif, TIFFTAG_IMAGELENGTH, &h);
            TIFFGetField( tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample );
            std::cout << "Bits per sample " << bitsPerSample  << std::endl;
            npixals = w * h;

            std::cout << "Height : " << h << std::endl;
            std::cout << "Width : " << w << std::endl;

            uint32* raster = (uint32*) _TIFFmalloc( npixals * sizeof(uint32) );
            if( raster != NULL )
            {
                if( TIFFReadRGBAImage( tif, w, h, raster, 0 )  )
                {
                    
#define READ_INVIVIDUAL_VALUES
#ifdef  READ_INVIVIDUAL_VALUES
                    Mat frame(w, h, CV_16U);
                    for (size_t i = 0; i < w; i++) 
                        for (size_t ii = 0; ii < h; ii++) 
                            frame.at<uint16_t>(i, ii) = toGray( raster[i*h+ii]);

                    // Rescale to 0 to 255.
                    frame.convertTo( frame, CV_8U, 1.0/32 );

#else      /* -----  not READ_INVIVIDUAL_VALUES  ----- */

                    Mat frame( w, h, CV_16U, raster );

#endif     /* -----  not READ_INVIVIDUAL_VALUES  ----- */

                    cout << frame.cols << " " << frame.rows << endl;
                    cout << endl << "====== " << endl;
                    cout << frame;
                    frames.push_back( frame );
                }

            }

	} while (TIFFReadDirectory(tif));
    }
    TIFFClose(tif);
#else 
    imreadmulti( String( filename.c_str() )
            , frames
            , IMREAD_ANYDEPTH
            );

    double maxPixalValue = 0;
    double minVal , maxVal;
    Point minLoc, maxLoc;
    for (size_t i = 0; i < frames.size(); i++) 
    {
        minMaxLoc( frames[i], &minVal, &maxVal, &minLoc, &maxLoc );
        if( maxVal > maxPixalValue )
            maxPixalValue = maxVal;
    }

    // Now rescale every frame to get the value between 0 and 255.
    if( maxPixalValue > 255.0 )
    {
        for (size_t i = 0; i < frames.size(); i++) 
        {
            Mat frame = frames[i];
            frame.convertTo( frame, CV_8U, 255.0 / maxPixalValue );
            frames[i] = frame;
        }
    }
#endif
}

void get_frames_from_avi( const string& filename
        , vector< Mat >& frames 
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

void read_frames( const string& filename
        , vector< Mat >& frames 
        , video_info_t& vidInfo
        )
{

    string::size_type pAt = filename.find_last_of('.');       
    string ext = filename.substr( pAt+1 );
    std::cout << "[INFO] Extenstion of file " << ext << std::endl;

    if( ext == "tif" || ext == "tiff" )
    {
        std::cout << "[INFO] Got a tiff file" << std::endl;
        get_frames_from_tiff( filename, frames, vidInfo );
    }
    else if( ext == "avi" )
    {
        std::cout << "[INFO] Got a avi file" << std::endl;
        get_frames_from_avi( filename, frames, vidInfo );
    }

}

void write_frames_to_avi( const string& avifile
        , vector< Mat > frames 
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
