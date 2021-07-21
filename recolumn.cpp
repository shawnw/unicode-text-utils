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

#include <memory>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <unicode/regex.h>
#include <unicode/ucnv.h>
#include <unicode/unistr.h>
#include <unicode/ustdio.h>

#include <getopt.h>

#include "formatter.h"

using namespace std::literals::string_literals;

const char *version = "0.1";

void print_usage(const char *name) {
  std::cout
      << name << " [ARGS] [FILE ...]\n\n"
      << "ARGS:\n"
      << " -h/--help\t\tDisplay this information.\n"
      << " -v/--version\t\tDisplay version.\n"
      << " -d/--delimiter=RE\tSet the column separator regular expression.\n"
      << " -c/--colspec=SPEC\tSet the column specification.\n"
      << " -l/--list\t\tUse list mode output.\n";
}

using colvector = std::vector<icu::UnicodeString>;

class line_breaker {
private:
  std::unique_ptr<icu::RegexPattern> pattern;
  std::unique_ptr<icu::RegexMatcher> splitter;
  colvector fields;
  bool getline(UFILE *, icu::UnicodeString *out);

public:
  line_breaker(const icu::UnicodeString &re);
  bool split(UFILE *, colvector *out);
};

line_breaker::line_breaker(const icu::UnicodeString &re) {
  UParseError pe;
  UErrorCode err = U_ZERO_ERROR;
  pattern = std::unique_ptr<icu::RegexPattern>(
      icu::RegexPattern::compile(re, pe, err));
  if (U_FAILURE(err)) {
    throw std::invalid_argument{"Invalid regular expression: "s +
                                u_errorName(err)};
  }
  splitter = std::unique_ptr<icu::RegexMatcher>(pattern->matcher(err));
  if (U_FAILURE(err)) {
    throw std::runtime_error{"Couldn't create RegexMatcher: "s +
                             u_errorName(err)};
  }
}

bool line_breaker::getline(UFILE *uf, icu::UnicodeString *out) {
  UChar buffer[4096];
  out->remove();
  do {
    UChar *s = u_fgets(buffer, 4096, uf);
    if (!s) {
      return u_feof(uf) && !out->isEmpty();
    }
    out->append(s, -1);
    auto lastidx = out->length() - 1;
    if (out->charAt(lastidx) == '\n') {
      out->remove(lastidx);
      return true;
    }
  } while (1);
}

bool line_breaker::split(UFILE *uf, colvector *out) {
  icu::UnicodeString line;
  UErrorCode err = U_ZERO_ERROR;

  if (!getline(uf, &line)) {
    return false;
  }

  if (fields.size() < line.length()) {
    fields.resize(line.length());
  }
  auto nfields = splitter->split(line, &fields[0], fields.size(), err);
  if (U_FAILURE(err)) {
    return false;
  }
  out->assign(fields.begin(), fields.begin() + nfields);
  return true;
}

int main(int argc, char **argv) {
  enum output_type { OUT_COLUMN, OUT_LIST } out_type = OUT_COLUMN;

  struct option opts[] = {
      {"version", 0, nullptr, 'v'},   {"help", 0, nullptr, 'h'},
      {"delimiter", 1, nullptr, 'd'}, {"colspec", 1, nullptr, 'c'},
      {"list", 0, nullptr, 'l'},      {nullptr, 0, nullptr, 0}};
  const char *split_re = "\\s+";
  const char *colspec = nullptr;

  for (int val;
       (val = getopt_long(argc, argv, "vhd:c:l", opts, nullptr)) != -1;) {
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
    case 'l':
      out_type = OUT_LIST;
      break;
    case '?':
    default:
      return 1;
    }
  }

  try {
    UErrorCode err = U_ZERO_ERROR;

    icu::UnicodeString usplit_re{split_re, -1, nullptr, err};
    if (U_FAILURE(err)) {
      throw std::runtime_error{"Couldn't convert '"s + split_re +
                               "' to unicode: "s + u_errorName(err)};
    }

    line_breaker breaker{usplit_re};
    colvector fields;

    std::unique_ptr<UFILE, decltype(&u_fclose)> ustdin{
        u_fadopt(stdin, nullptr, nullptr), &u_fclose};
    if (!ustdin) {
      throw std::runtime_error{"Unable to read from standard input"};
    }

    if (out_type == OUT_LIST) {
      auto fmt = make_list_formatter();
      while (breaker.split(ustdin.get(), &fields)) {
        fmt->format_line(fields);
      }
      fmt->flush();
    } else {
      int line = 0;
      while (breaker.split(ustdin.get(), &fields)) {
        u_printf("Line %d has %d fields\n", ++line, (int)fields.size());
      }
    }

  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
  }

  return 0;
}
