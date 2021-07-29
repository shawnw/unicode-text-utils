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
#include <fstream>
#include <string>
#include <memory>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <cstdlib>

#include <unicode/normalizer2.h>
#include <unicode/stringpiece.h>
#include <unicode/bytestream.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>

const char *version = "0.1";

using namespace std::literals::string_literals;

class output_bytesink : public icu::ByteSink {
private:
  int fd;

public:
  output_bytesink(int fd_) : fd(fd_) {}
  ~output_bytesink() noexcept override {}
  void Append(const char *, int32_t) override;
};

void output_bytesink::Append(const char *bytes, int32_t len) {
  ssize_t out;
  while ((out = write(fd, bytes, len)) >= 0 && out < len) {
    bytes += out;
    len -= out;
  }
  if (out < 0) {
    throw std::runtime_error{"Unable to write to standard output: "s +
                             std::strerror(errno)};
  }
}

class file_wrapper {
private:
  int fd;

public:
  file_wrapper(int fd_) : fd(fd_) {}
  ~file_wrapper() noexcept {
    if (fd >= 0) {
      close(fd);
    }
  }
  operator int() { return fd; }
};

bool try_mmap_norm(const char *filename, const icu::Normalizer2 *method,
                   icu::ByteSink &bs) {
  file_wrapper fd(open(filename, O_RDONLY));
  if (fd < 0) {
    throw std::invalid_argument{filename};
  }

  struct stat s;
  if (fstat(fd, &s) < 0) {
    return false;
  }

  auto free_mmap = [&s](void *mem) {
    if (mem != MAP_FAILED) {
      munmap(mem, s.st_size);
    }
  };

  std::unique_ptr<void, decltype(free_mmap)> utf8(
      mmap(nullptr, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0), free_mmap);
  if (utf8.get() == MAP_FAILED) {
    return false;
  }

  icu::StringPiece sp(static_cast<char *>(utf8.get()), s.st_size);
  UErrorCode err = U_ZERO_ERROR;

  method->normalizeUTF8(0, sp, bs, nullptr, err);

  if (U_FAILURE(err)) {
    throw std::runtime_error{"Unable to normalize text: "s + u_errorName(err)};
  }

  return true;
}

bool try_line_norm(const char *filename, const icu::Normalizer2 *method,
                   icu::ByteSink &bs) {
  std::string line;
  ssize_t len;
  std::string normalized;
  UErrorCode err = U_ZERO_ERROR;
  std::ifstream inf{filename};

  if (!inf.is_open()) {
    throw std::invalid_argument{filename};
  }

  while (std::getline(inf, line)) {
    line += '\n';
    icu::StringPiece sp(line.data(), line.size());
    method->normalizeUTF8(0, sp, bs, nullptr, err);
    if (U_FAILURE(err)) {
      break;
    }
  }

  if (U_FAILURE(err)) {
    throw std::runtime_error{"Unable to normalize text: "s + u_errorName(err)};
  }

  return true;
}

void do_normalization(const char *filename, const icu::Normalizer2 *method,
                      icu::ByteSink &bs) {
  if (try_mmap_norm(filename, method, bs)) {
    return;
  }
  if (try_line_norm(filename, method, bs)) {
    return;
  }
  throw std::runtime_error{"Unable to normalize file '"s + filename + "'"s};
}

int main(int argc, char **argv) {
  struct option opts[] = {
      {"version", 0, nullptr, 'v'}, {"help", 0, nullptr, 'h'},
      {"nfc", 0, nullptr, 1},       {"nfd", 0, nullptr, 2},
      {"nfkc", 0, nullptr, 3},      {"nfkd", 0, nullptr, 4},
      {nullptr, 0, nullptr, 0}};

  const icu::Normalizer2 *method = nullptr;
  UErrorCode err = U_ZERO_ERROR;

  for (int val; (val = getopt_long(argc, argv, "vh", opts, nullptr)) != -1;) {
    switch (val) {
    case 'v':
      std::cout << argv[0] << " version " << version << '\n';
      return 0;
    case 'h':
      std::cout << argv[0] << "--nfc|--nfd|--nfkc|--nfkd [FILE ...]\n";
      return 0;
    case 1:
      if (method) {
        std::cerr << argv[0] << ": can only specify one normalization mode.\n";
        return 1;
      }
      method = icu::Normalizer2::getNFCInstance(err);
      break;
    case 2:
      if (method) {
        std::cerr << argv[0] << ": can only specify one normalization mode.\n";
        return 1;
      }
      method = icu::Normalizer2::getNFDInstance(err);
      break;
    case 3:
      if (method) {
        std::cerr << argv[0] << ": can only specify one normalization mode.\n";
        return 1;
      }
      method = icu::Normalizer2::getNFKCInstance(err);
      break;
    case 4:
      if (method) {
        std::cerr << argv[0] << ": can only specify one normalization mode.\n";
        return 1;
      }
      method = icu::Normalizer2::getNFKDInstance(err);
      break;
    case '?':
    default:
      return 1;
    }
  }

  if (!method) {
    std::cerr << argv[0] << ": No normalization mode given.\n";
    return 1;
  }

  if (U_FAILURE(err)) {
    std::cerr << argv[0] << ": Unable to open normalizer: " << u_errorName(err)
              << '\n';
    return 1;
  }

  try {
    output_bytesink bs(STDOUT_FILENO);

    if (optind == argc) {
      do_normalization("/dev/stdin", method, bs);
    } else {
      for (int i = optind; i < argc; i += 1) {
        try {
          do_normalization(argv[i], method, bs);
        } catch (std::invalid_argument &) {
          std::cerr << "Unable to open '" << argv[i] << "' for reading.\n";
        }
      }
    }
  } catch (std::exception &e) {
    std::cerr << argv[0] << ": " << e.what() << '\n';
    return 1;
  }

  return 0;
}
