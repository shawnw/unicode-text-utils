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

#include <unordered_map>

#include <unicode/char16ptr.h>
#include <unicode/unistr.h>
#include <unicode/schriter.h>
#include <unicode/ustdio.h>

#include "util.h"

// Return the number of fixed-width columns taken up by a unicode codepoint
// Inspired by https://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c
int uu::unicwidth(UChar32 c) {
  static std::unordered_map<UChar32, int> cache;

  auto it = cache.find(c);
  if (it != cache.end()) {
    return it->second;
  }

  if (c == 0 || c == 0x200B) { // nul and ZERO WIDTH SPACE
    cache.emplace(c, 0);
    return 0;
  } else if (c >= 0x1160 && c <= 0x11FF) { // Hangul Jamo vowels and
                                           // final consonants
    cache.emplace(c, 0);
    return 0;
  } else if (c == 0xAD) { // SOFT HYPHEN
    cache.emplace(c, 1);
    return 1;
  } else if (u_isISOControl(c)) {
    cache.emplace(c, 0);
    return 0;
  }

  int type = u_charType(c);
  if (type == U_NON_SPACING_MARK || type == U_ENCLOSING_MARK ||
      type == U_FORMAT_CHAR) {
    cache.emplace(c, 0);
    return 0;
  }

  switch (u_getIntPropertyValue(c, UCHAR_EAST_ASIAN_WIDTH)) {
  case U_EA_FULLWIDTH:
  case U_EA_WIDE:
    cache.emplace(c, 2);
    return 2;
  case U_EA_HALFWIDTH:
  case U_EA_NARROW:
  case U_EA_NEUTRAL:
  case U_EA_AMBIGUOUS:
    cache.emplace(c, 1);
    return 1;
  default:
    cache.emplace(c, 1);
    return 1;
  }

  return -1;
}

int uu::unicswidth(const icu::UnicodeString &s) {
  auto iter = icu::StringCharacterIterator{s};
  int width = 0;
  for (UChar32 c = iter.first32PostInc(); c != StringCharacterIterator::DONE;
       c = iter.next32PostInc()) {
    width += uu::unicwidth(c);
  }
  return width;
}

bool uu::getline(UFILE *uf, icu::UnicodeString *out, bool flush, bool keepnl) {
  UChar buffer[4096];
  if (flush) {
    out->remove();
  }

  bool first = true;

  do {
    UChar *s = u_fgets(buffer, 4095, uf);
    if (!s) {
      return u_feof(uf) && !out->isEmpty();
    }
    out->append(s, -1);
    if (out->endsWith(u"\n", 0, 1)) {
      if (!keepnl) {
        out->remove(out->length() - 1);
      }
      return true;
    }
  } while (1);
}

bool uu::getparagraph(UFILE *uf, icu::UnicodeString *out, bool flush) {
  if (flush) {
    out->remove();
  }

  icu::UnicodeString line;
  bool first = true;

  do {
    if (uu::getline(uf, &line)) {
      if (line.isEmpty()) {
        return true;
      } else {
        if (first) {
          first = false;
        } else {
          out->append(u' ');
        }
        out->append(line);
      }
    } else {
      return u_feof(uf) && !out->isEmpty();
    }
  } while (1);
}
