uwc
===

A version of `wc(1)` that counts Unicode codepoints and extended
grapheme clusters, and uses Unicode word-breaking algorithm to
determine word count.

Running uwc
-----------

`uwc` takes 0 or more options, and 0 or more files to print counts
of. Reads from standard input if no files are given.

The following options may be used to select which counts are printed,
always in the following order and separated by tabs: newline, word,
codepoint, maximum line length.

* `--codepoints`/`-c` prints the codepoint counts.
* `--chars`/`-m` prints the character (Extended grapheme cluster) counts.
* `--lines`/`-l` prints the newline counts.
* `--max-line-length`/`-L` prints the maximum display width.
* `--words`/`-w` prints the word counts.

If none are specified, acts like `--lines --words --chars` was given.

It also understands the following options:

* `--version`/`-v` Print out the version and exit.
* `--help`/`-h` Print out usage information and exit.
