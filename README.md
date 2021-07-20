recolumn
========

A program to (semi-)intelligently reformat columnar data.

Running recolumn
----------------

`recolumn` takes 0 or more options, and 0 or more files to format as
command-line arguments. If no files are given, reads from standard
input. Always writes to standard output. All rows in a given run
should have the same number of columns.

### Options ###

  * `-h`/`--help` prints out usage information.
  * `-v`/`--version` prints the program version.
  * `-dRE`/`--delimiter=RE` sets a regular expression to use as the
    delimiter between columns. Defaults to whitespace.
  * `-cSPEC`/`--colspec=SPEC` sets the column format
    specification. More on that later.

### Column format specification ###

A column format specification is a comma-separated list of formats,
one per column in the input.

TODO: Document format

If no specification is given, defaults to automatically determining
the column widths based on the longest value of each column, and
left-justfication within each column.


Build Instructions
------------------

Requirements: A C++ compiler, cmake, ICU libraries and header files.

From the root project directory:

    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make
