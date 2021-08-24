usplit
======

Split up text according to various boundaries.

Takes one option, and 0 or more filenmaes. Reads from standard input
if no files are given on the commnad line. Always writes to standard
output.

Options
-------

One of the following boundary types is required:

* `--codepoints`/`-c`
* `--chars`/`-m`
* `--words`/`-w`
* `--sentences`/`-s`

Also understands the following flags (Mandatory arguments for long
options are also mandatory for short ones):

* `--version`/`-v` Print out the version and exit.
* `--help`/`-h` Print out usage information and exit.
* `--delimiter=STR`/`-d` Use `STR` as the delimiter instead of newline.
* `--zero`/`-z` Use 0 bytes as delimiters.
* `--json`/`-j` Ouput a JSON array of strings (Or numbers for
  `--codepoints`. If multiple input files are given, each array is on
  its own line.
