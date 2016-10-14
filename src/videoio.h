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

#include <vector>
#include <tiffio.h>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;
#define FOURCC_CODEC_DEFAULT CV_FOURCC( 'M', 'J', 'P', 'G' )
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
                          );

void get_frames_from_avi ( const string& filename
                           , vector< Mat >& frames
                           , video_info_t& vidInfo
                         );

void read_frames ( const string& filename
                   , vector< Mat >& frames
                   , video_info_t& vidInfo
                 );


void write_frames( 
        const string& outfile                   /* Output file */
        , const vector< Mat > frames            /* All the frames */
        , const string& infile                  /* Input file. */
        );

/**
 * @brief Write frames to tiff file.
 *
 * @param 
 * @param {
 */
void write_frames_to_tiff ( const string& outfile
        , const vector< Mat > frames
        , const string& infile 
        );

#endif   /* ----- #ifndef videoio_INC  ----- */
