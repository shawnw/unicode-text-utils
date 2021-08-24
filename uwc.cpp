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
#include <stdexcept>
#include <memory>
#include <algorithm>
#include <cstring>

#include <unicode/unistr.h>
#include <unicode/ustdio.h>
#include <unicode/brkiter.h>
#include <unicode/locid.h>

#include <getopt.h>

#include "json.hpp"
#include "util.h"

using namespace std::literals::string_literals;

using ufp = std::unique_ptr<UFILE, decltype(&u_fclose)>;

const char *version = "0.2";
enum wanted_stats {
  WC_CP = 0x1,
  WC_CHAR = 0x2,
  WC_WORD = 0x4,
  WC_NL = 0x8,
  WC_LEN = 0x10,
  WC_PRINT = 0x20
};

struct counts {
  unsigned int flags;
  unsigned int cp, chars, word, nl, len;
  counts() : flags(0), cp(0), chars(0), word(0), nl(0), len(0) {}
  counts(unsigned int flags_)
      : flags(flags_), cp(0), chars(0), word(0), nl(0), len(0) {}
};

std::ostream &operator<<(std::ostream &os, const struct counts &c) {
  bool first = true;
  if (c.flags & WC_NL) {
    os << c.nl;
    first = false;
  }
  if (c.flags & WC_WORD) {
    if (!first) {
      os << '\t';
    }
    os << c.word;
    first = false;
  }
  if (c.flags & WC_CHAR) {
    if (!first) {
      os << '\t';
    }
    os << c.chars;
    first = false;
  }
  if (c.flags & WC_CP) {
    if (!first) {
      os << '\t';
    }
    os << c.cp;
    first = false;
  }
  if (c.flags & WC_LEN) {
    if (!first) {
      os << '\t';
    }
    os << c.len;
    first = false;
  }
  return os;
}

nlohmann::json counts_to_json(const char *filename, const struct counts &c) {
  nlohmann::json res;
  res["filename"] = filename;
  if (c.flags & WC_CP) {
    res["codepoints"] = c.cp;
  }
  if (c.flags & WC_CHAR) {
    res["characters"] = c.chars;
  }
  if (c.flags & WC_WORD) {
    res["words"] = c.word;
  }
  if (c.flags & WC_NL) {
    res["newlines"] = c.nl;
  }
  if (c.flags & WC_LEN) {
    res["max-line-length"] = c.len;
  }
  return res;
}

nlohmann::json count(UFILE *uf, const char *filename, unsigned int flags,
                     struct counts &total_counts, const icu::Locale &loc) {
  struct counts counts(flags);
  UErrorCode err = U_ZERO_ERROR;
  icu::UnicodeString line;
  std::unique_ptr<icu::BreakIterator> wit, cit;

  if (flags & WC_WORD) {
    wit = std::unique_ptr<icu::BreakIterator>{
        icu::BreakIterator::createWordInstance(loc, err)};
    if (U_FAILURE(err)) {
      throw std::runtime_error{"Unable to create word break iterator: "s +
                               u_errorName(err)};
    }
  }

  if (flags & WC_CHAR) {
    cit = std::unique_ptr<icu::BreakIterator>{
        icu::BreakIterator::createCharacterInstance(loc, err)};
    if (U_FAILURE(err)) {
      throw std::runtime_error{"Unable to create character break iterator: "s +
                               u_errorName(err)};
    }
  }

  while (uu::getline(uf, &line, true, true)) {
    if (flags & WC_CP) {
      auto cps = line.countChar32();
      counts.cp += cps;
      total_counts.cp += cps;
    }

    if (flags & WC_NL && line.endsWith(u"\n", 0, 1)) {
      counts.nl += 1;
      total_counts.nl += 1;
    }

    if (flags & WC_LEN) {
      unsigned int len = uu::unicswidth(line);
      counts.len = std::max(counts.len, len);
      total_counts.len = std::max(total_counts.len, len);
    }

    if (flags & WC_WORD) {
      wit->setText(line);
      for (auto pos = wit->first(); pos != icu::BreakIterator::DONE;
           pos = wit->next()) {
        if (wit->getRuleStatus() != UBRK_WORD_NONE) {
          counts.word += 1;
          total_counts.word += 1;
        }
      }
    }

    if (flags & WC_CHAR) {
      cit->setText(line);
      for (auto pos = cit->first(); pos != icu::BreakIterator::DONE;
           pos = cit->next()) {
        counts.chars += 1;
        total_counts.chars += 1;
      }
    }
  }

  if (flags & WC_PRINT) {
    std::cout << counts;
    if (filename) {
      std::cout << '\t' << filename;
    }
    std::cout << '\n';
  }

  return counts_to_json(filename, counts);
}

void print_usage(const char *progname) {
  std::cout << "Usage: " << progname << " [OPTION ...] [FILE ...]\n";
  std::cout << R"(
Print newline, word, character and codepoint counts for each FILE, and
a total line if more than one FILE is specified. Words are defined by
the Unicode word-breaking algorithm and characters are Unicode
extended grapheme clusters.

With no FILE, or when FILE is -, read standard input.

The options below may be used to select which counts are printed,
always in the following order: newline, word, character, codepoint,
maximum line length.

  -c, --codepoints
  -m, --chars
  -l, --lines
  -L, --max-line-length
  -w, --words

When no options are given, acts like --lines --words --chars was
given.


Other options:

  -j, --json : print out an array of JSON objects instead.
  -v, --version : print out version and exit.
  -h, --help : print out usage information and exit.
)";
}

int main(int argc, char **argv) {
  struct option opts[] = {{"version", 0, nullptr, 'v'},
                          {"help", 0, nullptr, 'h'},
                          {"codepoints", 0, nullptr, 'c'},
                          {"chars", 0, nullptr, 'm'},
                          {"lines", 0, nullptr, 'l'},
                          {"words", 0, nullptr, 'w'},
                          {"max-line-length", 0, nullptr, 'L'},
                          {"json", 0, nullptr, 'j'},
                          {nullptr, 0, nullptr, 0}};
  unsigned int flags = 0;
  bool as_json = false;

  for (int val;
       (val = getopt_long(argc, argv, "vhcmlwLj", opts, nullptr)) != -1;) {
    switch (val) {
    case 'v':
      std::cout << argv[0] << " version " << version << '\n';
      return 0;
    case 'h':
      print_usage(argv[0]);
      return 0;
    case 'c':
      flags |= WC_CP;
      break;
    case 'm':
      flags |= WC_CHAR;
      break;
    case 'l':
      flags |= WC_NL;
      break;
    case 'w':
      flags |= WC_WORD;
      break;
    case 'L':
      flags |= WC_LEN;
      break;
    case 'j':
      as_json = true;
      break;
    default:
      return 1;
    }
  }

  if (!flags) {
    flags = WC_CHAR | WC_WORD | WC_NL;
  }

  if (!as_json) {
    flags |= WC_PRINT;
  }

  try {
    struct counts total_counts(flags);
    int nfiles = 0;
    icu::Locale loc;
    nlohmann::json results;

    if (optind == argc) {
      ufp ustdin{u_fadopt(stdin, nullptr, nullptr), &u_fclose};
      if (!ustdin) {
        throw std::runtime_error{"Unable to read from standard input"};
      }
      auto res = count(ustdin.get(), nullptr, flags, total_counts, loc);
      if (as_json) {
        results.push_back(res);
      }
      nfiles += 1;
    } else {
      for (int i = optind; i < argc; i += 1) {
        try {
          ufp uf = std::move([&]() {
            if (std::strcmp(argv[i], "/dev/stdin") == 0 ||
                std::strcmp(argv[i], "-") == 0) {
              return ufp{u_fadopt(stdin, nullptr, nullptr), &u_fclose};
            } else {
              return ufp{u_fopen(argv[i], "r", nullptr, nullptr), &u_fclose};
            }
          }());
          if (!uf) {
            throw std::invalid_argument{argv[i]};
          }
          auto res = count(uf.get(), argv[i], flags, total_counts, loc);
          if (as_json) {
            results.push_back(res);
          }
          nfiles += 1;
        } catch (std::invalid_argument &) {
          std::cerr << argv[0] << ": unable to open '" << argv[i] << "'\n";
        }
      }
    }
    if (as_json) {
      std::cout << results << '\n';
    }
    if (nfiles > 1 && !as_json) {
      std::cout << total_counts << "\ttotal\n";
    }
  } catch (std::exception &e) {
    std::cerr << argv[0] << ": " << e.what() << '\n';
    return 1;
  }

  return 0;
}
