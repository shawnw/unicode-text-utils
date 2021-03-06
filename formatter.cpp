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

#include "formatter.h"
#include "util.h"

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
      int w = uu::unicswidth(line[n]);
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
