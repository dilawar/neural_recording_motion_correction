/*
 * =====================================================================================
 *
 *       Filename:  stabilizer.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/17/2016 10:24:11 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dilawar Singh (), dilawars@ncbs.res.in
 *   Organization:  NCBS Bangalore
 *
 * =====================================================================================
 */

#ifndef  motion_stabilizer_INC
#define  motion_stabilizer_INC

#include "globals.h"


// This video stablisation smooths the global trajectory using a sliding average
// window In frames. The larger the more stable the video, but less reactive to
// sudden panning
const int SMOOTHING_RADIUS = 50;

// In pixels. Crops the border to reduce the black borders from stabilisation
// being too noticeable.
const int HORIZONTAL_BORDER_CROP = 10;

// 1. Get previous to current frame transformation (dx, dy, da) for all frames
// 2. Accumulate the transformations to get the image trajectory
// 3. Smooth out the trajectory using an averaging window
// 4. Generate new set of previous to current transform, such that the trajectory ends up being the same as the smoothed trajectory
// 5. Apply the new transformation to the video

struct TransformParam
{
    TransformParam() {}
    TransformParam(double _dx, double _dy, double _da)
    {
        dx = _dx;
        dy = _dy;
        da = _da;
    }

    double dx;
    double dy;
    double da; // angle
};

struct Trajectory
{
    Trajectory() {}
    Trajectory(double _x, double _y, double _a)
    {
        x = _x;
        y = _y;
        a = _a;
    }

    double x;
    double y;
    double a; // angle
};


/**
 * @brief Stablize the stack of frames.
 *
 * @param frames
 * @param result
 */
void stabilize( const vector< Mat >& frames , vector<Mat >& result );


#endif   /* ----- #ifndef motion_stabilizer_INC  ----- */
