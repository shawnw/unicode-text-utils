recolumn
========

A program to (semi-)intelligently reformat columnar data. It allows
more control over the output than the GNU coreutils `column -t`,
without having to come up with `printf()` formats or the like by hand.

It also strives to be Unicode-aware; for example, combining characters
are not counted towards the length of a field.

Running recolumn
----------------

`recolumn` takes 0 or more options, and 0 or more files to format as
command-line arguments. If no files are given, reads from standard
input. Always writes to standard output.

### Options ###

Mandatory arguments for long options are also mandatory for short ones.

  * `-h`/`--help` prints out usage information.
  * `-v`/`--version` prints the program version.
  * `-dRE`/`--delimiter=RE` sets a regular expression to use as the
    delimiter between columns. Defaults to whitespace.
  * `-cSPEC`/`--colspec=SPEC` sets the column format
    specification. More on that later.
  * `-l`/`--list` runs in list output mode instead of column output mode.

`recolumn` has several output modes.

### List Mode ###

Formats input fields into human-readable lists. An example might be best:

    $ echo "a b c" | recolumn --list
    a, b, and c

The exact format of the output lines depends on the current locale:

    $ echo "a b c" | LC_ALL=es_ES.UTF-8 recolumn --list
    a, b y c

### Column Mode ###

The default; prints output in fixed-width lines, reformatting columns
in the input to fit, adjusting alignment, and so on. All lines should
have the same number of columns.

#### Column format specification ####

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
