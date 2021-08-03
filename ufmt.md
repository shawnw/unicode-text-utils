ufmt
====

A version of `fmt(1)` that uses Unicode line-breaking rules based on
the current locale to wrap input.

Running ufmt
------------

`ufmt` takes 0 or more options, and 0 or more files to format as
command-line arguments. If no files are given, reads from standard
input. Always writes to standard output.

### Options ####

Mandatory arguments for long options are also mandatory for short ones.

* `--width=NUM`/`-w` - how many columns to format text for. Defaults
  to 78. If `auto` and standard output is a tty, uses its width if possible.
* `--version`/`-v` - print out the version and exit.
* `--help`/`-h` - print out usage information and exit.
