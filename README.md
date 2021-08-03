Unicode Text Utils
==================

Assorted Unicode-aware command line utilties.

* [recolumn](recolumn.md) - Reformat column-oriented text in various ways.
* [unorm](unorm.md) - Normalize UTF-8 text.
* [ufmt](ufmt.md) - Word-wrap paragraphs according to Unicode rules.
* [uwc](uwc.md) - Count words, characters, etc. according to Unicode rules.

Character Encodings
-------------------

Unless otherwise specified, programs use the current locale to
determine character encodings, and convert to/from an internal Unicode
encoding. If they're unable to convert, they'll exit with an error message.

Build Instructions
------------------

Requirements: A C++14 compiler, cmake, ICU 60+ libraries and header files.

From the root project directory:

    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make
