/*
 * Copyright © 2021 Shawn Wagner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <iostream>

#include <getopt.h>

const char *version = "0.1";

void print_usage(const char *name) {
  std::cout
      << name << " [ARGS] [FILE ...]\n\n"
      << "ARGS:\n"
      << " -h/--help\t\tDisplay this information.\n"
      << " -v/--version\t\tDisplay version.\n"
      << " -d/--delimiter=RE\tSet the column separator regular expression.\n"
      << " -c/--colspec=SPEC\tSet the column specification.\n";
}

int main(int argc, char **argv) {
  struct option opts[] = {{"version", 0, nullptr, 'v'},
                          {"help", 0, nullptr, 'h'},
                          {"delimiter", 1, nullptr, 'd'},
                          {"colspec", 1, nullptr, 'c'},
                          {nullptr, 0, nullptr, 0}};
  const char *split_re = "\\s+";
  const char *colspec = nullptr;

  for (int val;
       (val = getopt_long(argc, argv, "vhd:c:", opts, nullptr)) != -1;) {
    switch (val) {
    case 'v':
      std::cout << argv[0] << " version " << version << '\n';
      return 0;
    case 'h':
      print_usage(argv[0]);
      return 0;
    case 'd':
      split_re = optarg;
      break;
    case 'c':
      colspec = optarg;
      break;
    case '?':
    default:
      return 1;
    }
  }

  return 0;
}
