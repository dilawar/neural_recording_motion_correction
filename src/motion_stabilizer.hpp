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
const int SMOOTHING_RADIUS = 30;

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

    double x;
    double y;
    double a; // angle
};


template< typename pixal_type_t>
void stabilize( vector< Mat_<pixal_type_t> >& frames
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

    for (size_t k = 1; k < frames.size(); k++)
    {
        prevGrey = frames[k-1];
        curGrey = frames[k];
        // vector from prev to cur
        vector <Point2f> prev_corner, cur_corner;
        vector <Point2f> prev_corner2, cur_corner2;
        vector <uchar> status;
        vector <float> err;

        goodFeaturesToTrack(prevGrey, prev_corner, 200, 0.01, 30);
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

        // translation + rotation only
        // false = rigid transform, no scaling/shearing
        Mat T = estimateRigidTransform(prev_corner2, cur_corner2, false);

        // in rare cases no transform is found. We'll just use the last known
        // good transform.
        if(T.data == NULL)
        {
            last_T.copyTo(T);
        }

        T.copyTo(last_T);

        // decompose T
        double dx = T.at<double>(0,2);
        double dy = T.at<double>(1,2);
        double da = atan2(T.at<double>(1,0), T.at<double>(0,0));

        prev_to_cur_transform.push_back(TransformParam(dx, dy, da));

#ifdef  DEBUG
        out_transform << k << " " << dx << " " << dy << " " << da << endl;
#endif     /* -----  not DEBUG  ----- */


        curGrey.copyTo(prevGrey);

#ifdef DBEUG
        cout << "[DEBUG] Frame: " << k << "/" << frames.size()
            << " - good optical flow: " << prev_corner2.size() << endl;
#endif 

    }

    // Step 2 - Accumulate the transformations to get the image trajectory

    // Accumulated frame to frame transform
    double a = 0;
    double x = 0;
    double y = 0;

    vector <Trajectory> trajectory; // trajectory at all frames

    for(size_t i=0; i < prev_to_cur_transform.size(); i++)
    {
        x += prev_to_cur_transform[i].dx;
        y += prev_to_cur_transform[i].dy;
        a += prev_to_cur_transform[i].da;

        trajectory.push_back(Trajectory(x,y,a));

#ifdef DEBUG
        out_trajectory << (i+1) << " " << x << " " << y << " " << a << endl;
#endif
    }

    // Step 3 - Smooth out the trajectory using an averaging window
    vector <Trajectory> smoothed_trajectory; // trajectory at all frames

    for(size_t i=0; i < trajectory.size(); i++)
    {
        double sum_x = 0;
        double sum_y = 0;
        double sum_a = 0;
        int count = 0;

        for(int j=-SMOOTHING_RADIUS; j <= SMOOTHING_RADIUS; j++)
        {
            if(i+j >= 0 && i+j < trajectory.size())
            {
                sum_x += trajectory[i+j].x;
                sum_y += trajectory[i+j].y;
                sum_a += trajectory[i+j].a;

                count++;
            }
        }

        double avg_a = sum_a / count;
        double avg_x = sum_x / count;
        double avg_y = sum_y / count;

        smoothed_trajectory.push_back(Trajectory(avg_x, avg_y, avg_a));

#ifdef DEBUG
        out_smoothed_trajectory << (i+1) << " " << avg_x
                                    << " " << avg_y << " " << avg_a << endl;
#endif 
    }

    // Step 4 - Generate new set of previous to current transform, such that the
    // trajectory ends up being the same as the smoothed trajectory
    vector <TransformParam> new_prev_to_cur_transform;

    // Accumulated frame to frame transform
    a = 0;
    x = 0;
    y = 0;

    for(size_t i=0; i < prev_to_cur_transform.size(); i++)
    {
        x += prev_to_cur_transform[i].dx;
        y += prev_to_cur_transform[i].dy;
        a += prev_to_cur_transform[i].da;

        // target - current
        double diff_x = smoothed_trajectory[i].x - x;
        double diff_y = smoothed_trajectory[i].y - y;
        double diff_a = smoothed_trajectory[i].a - a;

        double dx = prev_to_cur_transform[i].dx + diff_x;
        double dy = prev_to_cur_transform[i].dy + diff_y;
        double da = prev_to_cur_transform[i].da + diff_a;

        new_prev_to_cur_transform.push_back(TransformParam(dx, dy, da));

#ifdef DEBUG
        out_new_transform << (i+1) << " " << dx << " " << dy << " " << da << endl;
#endif
    }

    // Step 5 - Apply the new transformation to the video
    //inputVideo.set(CV_CAP_PROP_POS_FRAMES, 0);
    Mat T(2,3,CV_64F);

    // get the aspect ratio correct
    int vert_border = HORIZONTAL_BORDER_CROP * prevGrey.rows / prevGrey.cols;

    for( size_t k = 0; k < frames.size() -1; k ++ )
    {
        curGrey = frames[k];
        T.at<double>(0,0) = cos(new_prev_to_cur_transform[k].da);
        T.at<double>(0,1) = -sin(new_prev_to_cur_transform[k].da);
        T.at<double>(1,0) = sin(new_prev_to_cur_transform[k].da);
        T.at<double>(1,1) = cos(new_prev_to_cur_transform[k].da);

        T.at<double>(0,2) = new_prev_to_cur_transform[k].dx;
        T.at<double>(1,2) = new_prev_to_cur_transform[k].dy;

        Mat cur2;

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
