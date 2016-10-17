# Video Stabilizer 

See the demo [here on youtube](https://youtu.be/vGjIFvzOOQ8).

# Dependencies 

- libtiff 
- opencv 

# Build and install 

    $ git clone https://github.com/dilawar/video_stabilizer 
    $ cd video_stabilizer 
    $ cmake .
    $ make 
    $ sudo make install

# Usage 

    $ videostab -h 

Will print the help message on how to use the application.

# Supported formats 

## Input formats

- tiff 
- any format which opencv can decode.

## Output formats

- tiff (when input is a tiff file)
- avi  (when input is NOT a tiff file).

# Contact 

Email to dilawars@ncbs.res.in

# Credits 

- [Nghia Ho](http://nghiaho.com/?p=2093) for making the algorithm available on
  his blog. This application is build around it.
- Good people at stackoverflow.com

# TODO

- User manual 
