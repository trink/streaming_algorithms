# Streaming Algorithms

## Overview

### Count-min Sketch
The [Count-min sketch](https://en.wikipedia.org/wiki/Count%E2%80%93min_sketch)
calculates the frequency of an item in a stream.

### Matrix
[Matrix](https://trink.github.io/streaming_algorithms/lua_matrix.html)
data structure for a 2d matrix.

### Piecewise Parabolic Prediction (P2)
The [p2](http://www.cs.wustl.edu/~jain/papers/ftp/psqr.pdf) algorithm for
dynamic calculation of quantiles and histograms without storing observation.

### Running Stats
Calculates the mean, variance, and standard deviation
https://www.johndcook.com/blog/standard_deviation/


### Time Series
[Time series](https://trink.github.io/streaming_algorithms/lua_time_series.html)
data structure for windowed calculations.

[Full Documentation](http://trink.github.io/streaming_algorithms)

## Installation

### Prerequisites
* C compiler (GCC 4.7+, Visual Studio 2013)
* CMake (3.6+) - http://cmake.org/cmake/resources/software.html
* Git http://git-scm.com/download

#### Optional (used for documentation)
* Graphviz (2.28.0) - http://graphviz.org/Download.php
* Doxygen (1.8.11+)- http://www.stack.nl/~dimitri/doxygen/download.html#latestsrc
* gitbook (2.3) - https://www.gitbook.com/

### CMake Build Instructions

    git clone https://github.com/trink/streaming_algorithms.git
    cd streaming_algorithms
    mkdir release
    cd release

    # UNIX
    cmake -DCMAKE_BUILD_TYPE=Release -DCPACK_GENERATOR=[TGZ|RPM|DEB] ..
    make

    # Windows Visual Studio 2013
    cmake -DCMAKE_BUILD_TYPE=Release -G "NMake Makefiles" ..
    nmake

    ctest
