/*
    Copyright (c) 2008-2015 Jan W. Krieger (<jan@jkrieger.de>, <j.krieger@dkfz.de>), German Cancer Research Center (DKFZ) & IWR, University of Heidelberg

    last modification: $LastChangedDate: 2015-07-07 12:07:58 +0200 (Di, 07 Jul 2015) $  (revision $Rev: 4005 $)

    This software is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License (LGPL) as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/



/*
  Name: highrestimer.cpp
  Copyright: (c) 2007
  Author: Jan krieger <jan@jkrieger.de>, http://www.jkrieger.de/
*/

#include "tinytiffhighrestimer.h" // class's header file


HighResTimer::HighResTimer() {
  start();
}

HighResTimer::~HighResTimer() {
}

void HighResTimer::start(){
  #ifdef __LINUX__
  #endif
  #ifdef __WINDOWS__
      LARGE_INTEGER fr;
      QueryPerformanceFrequency(&fr);
      freq=(double)(fr.QuadPart);
      QueryPerformanceCounter(&last);
  #else
      gettimeofday(&last,0);
  #endif
};

double HighResTimer::get_time(){
  #ifdef __LINUX__
  #endif
  #ifdef __WINDOWS__
      LARGE_INTEGER now;
      QueryPerformanceCounter(&now);
      return ((double)(now.QuadPart-last.QuadPart)/freq)*1e6;
  #else
    struct timeval tv;
    gettimeofday(&tv,0);

    long long t1, t2;
    t1 = last.tv_sec * 1000000 + last.tv_usec;
    t2 = tv.tv_sec * 1000000 + tv.tv_usec;
    return abs(t2 - t1);


  #endif
};


void HighResTimer::test(double* mean, double* stddev, unsigned long* histogram, double* histogram_x, unsigned long histogram_size){
  unsigned long runs=1000000;
  double* h=(double*)malloc(runs*sizeof(double));
  *mean = 0;
  *stddev = 0;
  /* time measurement */
  double l=get_time(), n;
  for (unsigned long i=0; i<runs; i++) {
    n=get_time();
    h[i]=n-l;
    l=n;
    //printf("%lf\n",h[i]);
  }
  /* end measurement */

  /* compute mean value */
  double mymin=h[0];
  double mymax=h[0];
  for (unsigned long i=1; i<runs; i++) {
    *mean+=h[i]/(runs-1);
    if (h[i]>mymax) mymax=h[i];
    if (h[i]<mymin) mymin=h[i];
  }
  //printf("%lf   %lf\n", mymin, mymax);
  double binwidth=0;
  if (histogram_size>0) {
    binwidth=(mymax-mymin)/(histogram_size-1);
    for (unsigned int i=0; i<histogram_size; i++) {
      histogram[i]=0;
      histogram_x[i]=mymin+i*binwidth;
    }
  }

  /* compute standard deviation */
  for (unsigned long i=1; i<runs; i++) {
    (*stddev)=(*stddev)+((double)h[i]-*mean)*((double)h[i]-*mean)/(double)(runs-2);
    if (histogram_size>0) {
      unsigned long bin=(unsigned int)floor((h[i]-mymin)/binwidth);
      if (/*bin>=0 &&*/ bin<histogram_size) histogram[bin]++;
    }
  }
  *stddev=sqrt(*stddev);
  free(h);
};
