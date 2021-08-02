unorm
=====

Normalize UTF-8 text according to the given form.

Takes one option, and 0 or more filenames. Reads from standard input
if no files are given on the command line. Always writes to standard
output.

Options
-------

One of the following normalization modes is required:

* `--nfc`
* `--nfd`
* `--nfkc`
* `--nfkd`

Also understands:

* `--version`/`-v` Print out the version and exit.
* `--help`/`-h` - Print out usage information and exit.
* `--check`/`-c` - Instead of converting text, exits with error code 2
    if the input is **NOT** in the given normalization form.
