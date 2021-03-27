# Video Stabilizer 

See the demo [here on youtube](https://youtu.be/vGjIFvzOOQ8) (stabilised video is on the left).

![![Demo](https://img.youtube.com/vi/GjIFvzOOQ8/0.jpg)](https://youtu.be/vGjIFvzOOQ8)

This is optmized for recordings of calcium activity. It might even work on any recording which has small feature sizes. It 
can not correct large fluctuation.

# Dependencies 

Development package of following libraries.

- libtiff
- opencv 

# Build and install 

    $ git clone https://github.com/dilawar/video_stabilizer 
    $ cd video_stabilizer 
    $ cmake .
    $ make 
    $ sudo make install

# Usage 

Typical usage (with 4 passes).

    $ videostab -i /path/to/video -n 4 
    $ videostab -i /path/to/video -o /path/to/output -n 4

`videostab -h` will print the help message on how to use the application.

# Supported formats 

## Input formats

- tiff 
- any format which opencv can decode.

# Contact 

In case, you need any help,  mail to dilawars@ncbs.res.in

# Credits 

- [Nghia Ho](http://nghiaho.com/?p=2093) for making the algorithm available on
  his blog. This application is build around it.
- Good people at stackoverflow.com

# TODO

- User manual 
