/*
 * =====================================================================================
 *
 *       Filename:  motion_stabilizer.h
 *
 *    Description:  Motion stabilizer module.
 *
 *        Version:  1.0
 *        Created:  10/12/2016 02:25:52 PM
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

// This video stablisation smooths the global trajectory using a sliding average
// window In frames. The larger the more stable the video, but less reactive to
// sudden panning
const int SMOOTHING_RADIUS = 50;

// In pixels. Crops the border to reduce the black borders from stabilisation
// being too noticeable.
const int HORIZONTAL_BORDER_CROP = 20;

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

    /*-----------------------------------------------------------------------------
     *  This is from here: http://nghiaho.com/uploads/videostabKalman.cpp
     *-----------------------------------------------------------------------------*/
    // "+"
    friend Trajectory operator+(const Trajectory &c1,const Trajectory  &c2){
        return Trajectory(c1.x+c2.x,c1.y+c2.y,c1.a+c2.a);
    }
    //"-"
    friend Trajectory operator-(const Trajectory &c1,const Trajectory  &c2){
        return Trajectory(c1.x-c2.x,c1.y-c2.y,c1.a-c2.a);
    }
    //"*"
    friend Trajectory operator*(const Trajectory &c1,const Trajectory  &c2){
        return Trajectory(c1.x*c2.x,c1.y*c2.y,c1.a*c2.a);
    }
    //"/"
    friend Trajectory operator/(const Trajectory &c1,const Trajectory  &c2){
        return Trajectory(c1.x/c2.x,c1.y/c2.y,c1.a/c2.a);
    }
    //"="
    Trajectory operator =(const Trajectory &rx){
        x = rx.x;
        y = rx.y;
        a = rx.a;
        return Trajectory(x,y,a);
    }

    double x;
    double y;
    double a; // angle
};


template< typename pixal_type_t>
void stabilize( const vector< Mat_<pixal_type_t> >& frames
        , vector<Mat_<pixal_type_t> >& result )
{
    // For further analysis
#ifdef DEBUG
    ofstream out_transform("__prev_to_cur_transformation.txt");
    ofstream out_trajectory("__trajectory.txt");
    ofstream out_smoothed_trajectory("__smoothed_trajectory.txt");
    ofstream out_new_transform("__new_prev_to_cur_transformation.txt");
#endif

    // Step 1 - Get previous to current frame transformation (dx, dy, da) for all frames
    vector <TransformParam> prev_to_cur_transform; // previous to current
    Mat last_T;
    Mat curGrey, prevGrey;

    double a = 0.0, x = 0.0, y = 0.0;

    // Step 3 - Smooth out the trajectory using an averaging window
    vector <Trajectory> smoothed_trajectory; // trajectory at all frames
    Trajectory X; 
    Trajectory X_;
    Trajectory P;
    Trajectory P_;
    Trajectory K;                               /* Gain */
    Trajectory z;                               /* Actual measurement */
    double pstd = 4e-3;
    double cstd = 0.25;

    Trajectory Q( pstd, pstd, pstd );           /* Process noise covariance */
    Trajectory R( cstd, cstd, cstd);            /* Measurement noise covariance */

    X = Trajectory( 0, 0, 0);
    P = Trajectory( 1, 1, 1);

    for (size_t k = 1; k < frames.size(); k++)
    {
        prevGrey = frames[k-1];
        curGrey = frames[k];
        // vector from prev to cur
        vector <Point2f> prev_corner, cur_corner;
        vector <Point2f> prev_corner2, cur_corner2;
        vector <uchar> status;
        vector <float> err;

        /*-----------------------------------------------------------------------------
         *  Good features to track. This function has following signature.
         *
         *   void goodFeaturesToTrack(InputArray image
         *      , OutputArray corners, int maxCorners
         *      , double qualityLevel, double minDistance
         *      , InputArray mask=noArray(), int blockSize=3
         *      , bool useHarrisDetector=false, double k=0.04 
         *      )
         *
         *  No of maximum corners are 200 in our approach.
         *-----------------------------------------------------------------------------*/
        goodFeaturesToTrack(prevGrey, prev_corner, 100,  0.01, 30);
        calcOpticalFlowPyrLK(prevGrey, curGrey, prev_corner, cur_corner, status, err);

        // weed out bad matches
        for(size_t i=0; i < status.size(); i++)
        {
            if(status[i])
            {
                prev_corner2.push_back(prev_corner[i]);
                cur_corner2.push_back(cur_corner[i]);
            }
        }


        // Step 5 - Apply the new transformation to the video
        //inputVideo.set(CV_CAP_PROP_POS_FRAMES, 0);
        // translation + rotation only
        // false = rigid transform, no scaling/shearing
        Mat T = estimateRigidTransform(prev_corner2, cur_corner2, false);

        // in rare cases no transform is found. We'll just use the last known
        // good transform.
        if(T.data == NULL)
            last_T.copyTo(T);
        T.copyTo(last_T);

        // decompose T
        double dx = T.at<double>(0,2);
        double dy = T.at<double>(1,2);
        double da = atan2(T.at<double>(1,0), T.at<double>(0,0));

        //prev_to_cur_transform.push_back(TransformParam(dx, dy, da));

#ifdef  DEBUG
        out_transform << k << " " << dx << " " << dy << " " << da << endl;
#endif     /* -----  not DEBUG  ----- */


        // Accumulated frame to frame transform
        x += dx;
        y += dy;
        a += da;

#ifdef DEBUG
        out_trajectory << k << " " << x " " << y << " " << a << endl;
#endif 
        z = Trajectory( x, y, a);
        X_ = X; 
        P_ = P + Q; 
        // meaurement update correction
        K = P_ / (P_ + R );
        X = X_ + K * ( z - X_ );
        P = (Trajectory(1,1,1) - K) * P_;

#ifdef DEBUG 
        out_smoothed_trajectory << k << " " << X.x << " " << X.y << " " 
            << X.a << endl;
#endif

        // target - current
        double diffX = X.x - x;
        double diffY = X.y - y;
        double diffa = X.a - a;
        dx += diffX; dy += diffY; da += diffa;

#ifdef DEBUG 
        out_new_transform << k << " " << dx << " " << dy << " " << da << endl;
#endif 
        T.at<double>(0,0) = cos( da );
        T.at<double>(0,1) = -sin( da );
        T.at<double>(1,0) = sin( da );
        T.at<double>(1,1) = cos( da );

        T.at<double>(0,2) = dx;
        T.at<double>(1,2) = dy;

#ifdef DBEUG
        cout << "[DEBUG] Frame: " << k << "/" << frames.size()
            << " - good optical flow: " << prev_corner2.size() << endl;
#endif 

        // Accumulated frame to frame transform
        a = 0;
        x = 0;
        y = 0;

        Mat cur2;
        // get the aspect ratio correct
        int vert_border = HORIZONTAL_BORDER_CROP * prevGrey.rows / prevGrey.cols;

        warpAffine(curGrey, cur2, T, curGrey.size());

        cur2 = cur2( Range(vert_border, cur2.rows-vert_border)
                , Range(HORIZONTAL_BORDER_CROP, cur2.cols-HORIZONTAL_BORDER_CROP)
                );

        // Resize cur2 back to cur size, for better side by side comparison
        resize(cur2, cur2, curGrey.size());
        result.push_back( cur2 );
    }
}


#endif   /* ----- #ifndef motion_stabilizer_INC  ----- */
