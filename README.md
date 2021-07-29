Unicode Text Utils
==================

Assorted Unicode-aware command line utilties.


* [recolumn](recolumn.md) - Reformat column-oriented text in various ways.
* [unorm](unorm.md) - Normalize UTF-8 text.

Build Instructions
------------------

Requirements: A C++14 compiler, cmake, ICU 60+ libraries and header files.

From the root project directory:

    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make
