#include <sstream>
#include <iomanip>
#include <cassert>
#include "csapp.h"
#include "except.h"
#include "io.h"

namespace {

  // Helper function: put bytes from fd into message
  void put_to_buffer(int fd, char *buf, size_t byte_size) {
    size_t i = 0;
    // read bytes from file
    while (i < byte_size) {
      ssize_t my_byte = read(fd, buf + i, byte_size - i);
      // read in an end of file character
      if (my_byte == 0) {
        throw IOException("file ended early");
      }
      // handle invalid character
      if (my_byte < 0) {
        throw IOException("invalid character, read error");
      }
      i += my_byte;
    }
  }

  // Helper function: ensures that string message is valid
  bool ensure_valid_string(char *message) {
    // loop through string and make sure that all values are valid
    for (int i = 0; i < 4; i++) {
      if (message[i] < '0' || message[i] > '9')
        return false;
    }
    return true;
  }


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
  // put infd into new char array
  char message[5] = {0};
  put_to_buffer(infd, message, 4);

  // ensure that put_to_buffer created valid message
  if (!ensure_valid_string(message)) {
    throw IOException("string is not valid");
  }

  // ensure that message length is valid
  int my_length = std::stoi(std::string(message, 4));
  if (my_length < 1) {
    throw IOException("string length is not valid");
  }

  std::string message_1(my_length, '\0');
  put_to_buffer(infd, &message_1[0], my_length);
 
  // Last byte must be newline
  if (message_1.back() != '\n')
    throw IOException("missing terminating newline");
 
  // Strip the newline
  s = message_1.substr(0, my_length - 1);
}

} // end of IO namespace
