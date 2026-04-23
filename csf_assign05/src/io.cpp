#include <sstream>
#include <iomanip>
#include <cassert>
#include "csapp.h"
#include "except.h"
#include "io.h"

namespace {

// TODO: define helper functions


} // end of anonymous namespace for helper functions

namespace IO {

void send(const std::string &s, int outfd) {
  // must frame s with an end character
  size_t len = s.size() + 1;

  //ensure that length output is 4 digits with leading zeros
  std::ostringstream oss;
  oss << std::setw(4) << std::setfill('0') << len;
  std::string my_frame = oss.str() + s + "\n";

  //check if bytes were written correctly
  if (rio_writen(outfd, my_frame.data(), my_frame.size()) != ssize_t(my_frame.size())) {
    throw IOException("written frame size does not match expected frame size");
  }
}

void receive(int infd, std::string &s) {
  // TODO: implement
}

} // end of IO namespace
