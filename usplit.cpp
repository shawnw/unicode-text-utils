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
#include <cstring>

#include <unicode/unistr.h>
#include <unicode/ustdio.h>
#include <unicode/brkiter.h>
#include <unicode/locid.h>

#include <getopt.h>

#include "util.h"

using namespace std::literals::string_literals;

using ufp = std::unique_ptr<UFILE, decltype(&u_fclose)>;

const char *version = "0.1";

enum class split_at { CP, CHAR, WORD, SENTENCE, UNSPEC };

class splitter {
protected:
  icu::UnicodeString delim;
  void print_delim(UFILE *);

public:
  splitter(const icu::UnicodeString &delim_) : delim(delim_){};
  virtual ~splitter() {}
  virtual void split(UFILE *) = 0;
};

void splitter::print_delim(UFILE *uf) {
  if (delim.isEmpty()) {
    u_fputc(0, uf);
  } else {
    u_file_write(delim.getBuffer(), delim.length(), uf);
  }
}

class cp_splitter : public splitter {
public:
  cp_splitter(const icu::UnicodeString &delim_) : splitter(delim_) {}
  ~cp_splitter() override {}
  void split(UFILE *) override;
};

void cp_splitter::split(UFILE *uf) {
  UChar32 c;
  UFILE *ustdout = u_get_stdout();
  bool first = true;

  while ((c = u_fgetcx(uf)) != U_EOF) {
    if (c == 0xFFFFFFFF) {
      c = 0xFFFD;
    }
    if (!first) {
      print_delim(ustdout);
    }
    first = false;
    u_fputc(c, ustdout);
  }
}

class break_splitter : public splitter {
protected:
  std::unique_ptr<icu::BreakIterator> bi;
  virtual bool skip() { return false; }
  break_splitter(const icu::UnicodeString &delim_) : splitter(delim_) {}

public:
  break_splitter(split_at which, icu::Locale &loc,
                 const icu::UnicodeString &delim_);
  ~break_splitter() override {}
  void split(UFILE *) override;
};

break_splitter::break_splitter(split_at which, icu::Locale &loc,
                               const icu::UnicodeString &delim_)
    : splitter(delim_) {
  UErrorCode err = U_ZERO_ERROR;
  bi = std::unique_ptr<icu::BreakIterator>([&]() {
    switch (which) {
    case split_at::SENTENCE:
      return icu::BreakIterator::createSentenceInstance(loc, err);
    default:
      throw std::runtime_error{"Unsupported break iterator type"};
    }
  }());
  if (U_FAILURE(err)) {
    throw std::runtime_error{"Unable to create iterator: "s + u_errorName(err)};
  }
}

void break_splitter::split(UFILE *uf) {
  icu::UnicodeString para;
  int32_t offset = 0;
  UFILE *ustdout = u_get_stdout();

  while (uu::getparagraph(uf, &para, true, true)) {
    bi->setText(para);
    for (auto pos = bi->first(); pos != icu::BreakIterator::DONE;
         pos = bi->next()) {
      if (!skip()) {
        icu::UnicodeString token;
        para.extractBetween(offset, pos, token);
        if (offset > 0) {
          print_delim(ustdout);
        }
        u_file_write(token.getBuffer(), token.length(), ustdout);
      }
      offset = pos;
    }
  }
}

class charbreak_splitter : public splitter {
private:
  std::unique_ptr<icu::BreakIterator> bi;

public:
  charbreak_splitter(icu::Locale &loc, const icu::UnicodeString &delim_);
  ~charbreak_splitter() override {}
  void split(UFILE *) override;
};

charbreak_splitter::charbreak_splitter(icu::Locale &loc,
                                       const icu::UnicodeString &delim_)
    : splitter(delim_) {
  UErrorCode err = U_ZERO_ERROR;
  bi = std::unique_ptr<icu::BreakIterator>(
      icu::BreakIterator::createCharacterInstance(loc, err));
  if (U_FAILURE(err)) {
    throw std::runtime_error{"Unable to create iterator: "s + u_errorName(err)};
  }
}

void charbreak_splitter::split(UFILE *uf) {
  icu::UnicodeString line;
  int32_t offset = 0;
  UFILE *ustdout = u_get_stdout();

  while (uu::getline(uf, &line, true, true)) {
    bi->setText(line);
    for (auto pos = bi->first(); pos != icu::BreakIterator::DONE;
         pos = bi->next()) {
      icu::UnicodeString token;
      line.extractBetween(offset, pos, token);
      if (offset > 0) {
        print_delim(ustdout);
      }
      u_file_write(token.getBuffer(), token.length(), ustdout);
      offset = pos;
    }
  }
}

class wordbreak_splitter : public break_splitter {
protected:
  bool skip() override { return bi->getRuleStatus() == UBRK_WORD_NONE; }

public:
  wordbreak_splitter(icu::Locale &loc, const icu::UnicodeString &delim_);
  ~wordbreak_splitter() override {}
};

wordbreak_splitter::wordbreak_splitter(icu::Locale &loc,
                                       const icu::UnicodeString &delim_)
    : break_splitter(delim_) {
  UErrorCode err = U_ZERO_ERROR;
  bi = std::unique_ptr<icu::BreakIterator>(
      icu::BreakIterator::createWordInstance(loc, err));
  if (U_FAILURE(err)) {
    throw std::runtime_error{"Unable to create word iterator: "s +
                             u_errorName(err)};
  }
}

std::unique_ptr<splitter> make_splitter(split_at which, icu::Locale &loc,
                                        const icu::UnicodeString &delim) {
  switch (which) {
  case split_at::CP:
    return std::make_unique<cp_splitter>(delim);
  case split_at::WORD:
    return std::make_unique<wordbreak_splitter>(loc, delim);
  case split_at::CHAR:
    return std::make_unique<charbreak_splitter>(loc, delim);
  case split_at::SENTENCE:
    return std::make_unique<break_splitter>(which, loc, delim);
  default:
    throw std::runtime_error{"Unknown splitter type"};
  }
}

void print_usage(const char *progname) {
  std::cout << "Usage: " << progname << "[OPTIONS] SPLIT-TYPE [FILE ...]\n";
  std::cout << R"(
Split up input files (Or standard input if no files given) into
individual codepoints, characters, words
or sentences, with a given delimiter between each.

Split types (One of these must be given):

  -c, --codepoints: Split into individual Unicode codepoints.
  -m, --chars: Split into Unicode characters (Extended grapheme clusters)
  -w, --words: Split into words according to the Unicode word-breaking algorithm.
  -s, --sentence: Split into sentences according to the Unicode sentence-breaking algorithm.

Other options (Mandatory arguments for long options are mandatory for short ones too):

  -h, --help: Print usage information and exit.
  -v, --version: Print version and exit.
  -d, --delimiter=STRING: Print STRING between tokens. Defaults to newline. Understands standard backslash escape sequences.
  -z, --zero: Use a null byte as the delimiter.
)";
}

int main(int argc, char **argv) {
  struct option opts[] = {
      {"version", 0, nullptr, 'v'},    {"help", 0, nullptr, 'h'},
      {"codepoints", 0, nullptr, 'c'}, {"chars", 0, nullptr, 'm'},
      {"sentences", 0, nullptr, 's'},  {"words", 0, nullptr, 'w'},
      {"delimiter", 1, nullptr, 'd'},  {"zero", 0, nullptr, 'z'},
      {nullptr, 0, nullptr, 0}};
  auto which = split_at::UNSPEC;
  icu::UnicodeString delim{u"\n"};

  for (int val;
       (val = getopt_long(argc, argv, "vhcmswd:z", opts, nullptr)) != -1;) {
    switch (val) {
    case 'v':
      std::cout << argv[0] << " version " << version << '\n';
      return 0;
    case 'h':
      print_usage(argv[0]);
      return 0;
    case 'c':
      which = split_at::CP;
      break;
    case 'm':
      which = split_at::CHAR;
      break;
    case 'w':
      which = split_at::WORD;
      break;
    case 's':
      which = split_at::SENTENCE;
      break;
    case 'd':
      delim = icu::UnicodeString(optarg, -1, nullptr).unescape();
      break;
    case 'z':
      delim = u"";
      break;
    default:
      return 1;
    }
  }

  if (which == split_at::UNSPEC) {
    std::cerr << argv[0] << ": missing split type argument.\n";
    return 1;
  }

  try {
    icu::Locale loc;
    auto splitter = make_splitter(which, loc, delim);

    if (optind == argc) {
      ufp ustdin{u_fadopt(stdin, nullptr, nullptr), &u_fclose};
      if (!ustdin) {
        throw std::runtime_error{"Unable to read from standard input"};
      }
      splitter->split(ustdin.get());
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
          splitter->split(uf.get());
        } catch (std::invalid_argument &) {
          std::cerr << argv[0] << ": unable to open '" << argv[i] << "'\n";
        }
      }
    }
  } catch (std::exception &e) {
    std::cerr << argv[0] << ": " << e.what() << '\n';
    return 1;
  }

  return 0;
}
