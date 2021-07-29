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
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

#include <unicode/listformatter.h>
#include <unicode/ustdio.h>
#include <unicode/unistr.h>
#include <unicode/schriter.h>

#include "formatter.h"

using namespace std::literals::string_literals;

class list_formatter : public formatter {
private:
  UFILE *ustdout;
  std::unique_ptr<icu::ListFormatter> fmt;

public:
  list_formatter();
  ~list_formatter() override {}
  void format_line(const std::vector<icu::UnicodeString> &) override;
  void flush() override {}
};

list_formatter::list_formatter() : ustdout(u_get_stdout()) {
  UErrorCode err = U_ZERO_ERROR;
  fmt = std::unique_ptr<icu::ListFormatter>(
      icu::ListFormatter::createInstance(err));
  if (U_FAILURE(err)) {
    throw std::runtime_error{"Couldn't create list formatter: "s +
                             u_errorName(err)};
  }
}

void list_formatter::format_line(
    const std::vector<icu::UnicodeString> &fields) {
  UErrorCode err = U_ZERO_ERROR;
  icu::UnicodeString output;
  fmt->format(fields.data(), fields.size(), output, err);
  if (U_FAILURE(err)) {
    throw std::runtime_error{"Couldn't format list line: "s + u_errorName(err)};
  }
  u_fputs(output.getTerminatedBuffer(), ustdout);
}

uformatter make_list_formatter() { return std::make_unique<list_formatter>(); }

// Return the number of fixed-width columns taken up by a unicode codepoint
// Inspired by https://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c
static int unicwidth(UChar32 c) {
  if (c == 0 || c == 0x200B) { // nul and ZERO WIDTH SPACE
    return 0;
  } else if (c >= 0x1160 && c <= 0x11FF) { // Hangul Jamo vowels and
                                           // final consonants
    return 0;
  } else if (c == 0xAD) { // SOFT HYPHEN
    return 1;
  } else if (u_isISOControl(c)) {
    return 0;
  }

  int type = u_charType(c);
  if (type == U_NON_SPACING_MARK || type == U_ENCLOSING_MARK ||
      type == U_FORMAT_CHAR) {
    return 0;
  }

  switch (u_getIntPropertyValue(c, UCHAR_EAST_ASIAN_WIDTH)) {
  case U_EA_FULLWIDTH:
  case U_EA_WIDE:
    return 2;
  case U_EA_HALFWIDTH:
  case U_EA_NARROW:
  case U_EA_NEUTRAL:
  case U_EA_AMBIGUOUS:
    return 1;
  default:
    return 1;
  }

  return -1;
}

static int count_width(const icu::UnicodeString &s) {
  auto iter = icu::StringCharacterIterator{s};
  int width = 0;
  for (UChar32 c = iter.first32PostInc(); c != StringCharacterIterator::DONE;
       c = iter.next32PostInc()) {
    width += unicwidth(c);
  }
  return width;
}

class column_formatter : public formatter {
  std::vector<std::vector<icu::UnicodeString>> data;

public:
  column_formatter(){};
  ~column_formatter() override {}
  void format_line(const std::vector<icu::UnicodeString> &) override;
  void flush() override;
};

void column_formatter::format_line(
    const std::vector<icu::UnicodeString> &fields) {
  data.push_back(fields);
}

void column_formatter::flush() {
  if (data.empty()) {
    return;
  }

  std::vector<int> maxwidths(data[0].size(), 1);
  std::vector<std::vector<int>> widths;
  for (const auto &line : data) {
    if (maxwidths.size() < line.size()) {
      maxwidths.resize(line.size(), 1);
    }

    std::vector<int> linewidths(line.size(), 0);
    for (int n = 0; n < line.size(); n += 1) {
      int w = count_width(line[n]);
      linewidths[n] = w;
      maxwidths[n] = std::max(maxwidths[n], w);
    }
    widths.emplace_back(std::move(linewidths));
  }

  UFILE *ustdout = u_get_stdout();
  for (int i = 0; i < data.size(); i += 1) {
    auto &line = data[i];
    for (int n = 0; n < line.size(); n += 1) {
      if (n > 0) {
        u_fputc(' ', ustdout);
      }
      u_printf("%S", line[n].getTerminatedBuffer());
      for (int s = widths[i][n]; s < maxwidths[n]; s += 1) {
        u_fputc(' ', ustdout);
      }
    }
    u_fputc('\n', ustdout);
  }

  data.clear();
}

uformatter make_column_formatter() {
  return std::make_unique<column_formatter>();
}
