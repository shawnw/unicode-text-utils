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
#include <string>
#include <memory>
#include <stdexcept>
#include <cstring>

#include <unicode/unistr.h>
#include <unicode/locid.h>
#include <unicode/brkiter.h>
#include <unicode/ustdio.h>

#include <sys/ioctl.h>
#include <termios.h>
#include <getopt.h>
#include <unistd.h>

#include "util.h"

const char *version = "0.1";

using namespace std::literals::string_literals;

using ufp = std::unique_ptr<UFILE, decltype(&u_fclose)>;

int get_tty_width(int defwidth) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) < 0) {
    return defwidth;
  } else {
    return ws.ws_col;
  }
}

class word_wrapper {
private:
  int width;
  std::unique_ptr<icu::BreakIterator> iter;
  void wrap(icu::UnicodeString &);

public:
  word_wrapper(int);
  void fmt(UFILE *);
};

word_wrapper::word_wrapper(int width_) : width(width_) {
  UErrorCode err = U_ZERO_ERROR;
  iter = std::unique_ptr<icu::BreakIterator>(
      icu::BreakIterator::createLineInstance(icu::Locale::getDefault(), err));
  if (U_FAILURE(err)) {
    throw std::runtime_error{"Unable to create line break object: "s +
                             u_errorName(err)};
  }
}

void word_wrapper::wrap(icu::UnicodeString &para) {
  UErrorCode err = U_ZERO_ERROR;
  iter->setText(para);
  icu::UnicodeString line;
  int32_t parawidth = 0;
  int32_t offset = 0;
  for (int32_t pos = iter->first(); pos != icu::BreakIterator::DONE;
       pos = iter->next()) {
    icu::UnicodeString chunk;
    para.extractBetween(offset, pos, chunk);
    int32_t w = uu::unicswidth(chunk);
    if (parawidth + w <= width) {
      line.append(chunk);
      parawidth += w;
    } else {
      if (!line.isEmpty()) {
        u_printf("%S\n", line.getTerminatedBuffer());
      }
      line = chunk;
      parawidth = w;
    }
    offset = pos;
  }
  if (!line.isEmpty()) {
    u_printf("%S\n", line.getTerminatedBuffer());
  }
}

void word_wrapper::fmt(UFILE *uf) {
  icu::UnicodeString para;
  bool first = true;
  auto ustdout = u_get_stdout();

  while (uu::getparagraph(uf, &para)) {
    if (first) {
      first = false;
    } else {
      u_fputc('\n', ustdout);
    }
    wrap(para);
  }
}

int main(int argc, char **argv) {
  struct option opts[] = {{"version", 0, nullptr, 'v'},
                          {"width", 1, nullptr, 'w'},
                          {"help", 0, nullptr, 'h'},
                          {nullptr, 0, nullptr, 0}};
  int width = 78;

  for (int val; (val = getopt_long(argc, argv, "vhw:", opts, nullptr)) != -1;) {
    switch (val) {
    case 'v':
      std::cout << argv[0] << " version " << version << '\n';
      return 0;
    case 'h':
      std::cout << argv[0] << " [--width=(N|auto)] [FILE ...]\n\n"
                << "Word-wrap paragraphs of input text according to Unicode "
                   "line breaking rules.\n";
      return 0;
    case 'w':
      if (std::strcmp(optarg, "auto") == 0) {
        width = get_tty_width(78);
      } else {
        width = std::stoi(optarg);
      }
      break;
    }
  }

  try {
    word_wrapper ww{width};

    if (optind == argc) {
      ufp ustdin{u_fadopt(stdin, nullptr, nullptr), &u_fclose};
      if (!ustdin) {
        throw std::runtime_error{"Unable to read from standard input"};
      }
      ww.fmt(ustdin.get());
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
          ww.fmt(uf.get());
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
