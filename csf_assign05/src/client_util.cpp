#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include "csapp.h"
#include "message.h"
#include "wire.h"
#include "io.h"
#include "except.h"
#include "client_util.h"

std::string inputLogin() {
  std::string username;
  std::string password;
  
  std::cout << "username: " << std::flush;
  if (!std::getline(std::cin, username)) {
    throw IOException("failed to read username");
  }

  std::cout << "password: " << std::flush;
  if (!std::getline(std::cin, password)) {
    throw IOException("failed to read password");
  }

  return username + "/" + password;
}

int connectLogin(const char *host, const char *port, ClientMode mode) {
  int fd = -1;
  try {
    std::string cred = inputLogin();

    fd = open_clientfd(host, port);
    if (fd < 0) {
      throw IOException("could not connect to server");
    }

    Message req(MessageType::LOGIN, mode, cred);
    std::string out;
    Wire::encode(req, out);
    IO::send(out, fd);

    std::string in;
    IO::receive(fd, in);
    Message resp;
    Wire::decode(in, resp);

    if (resp.get_type() == MessageType::ERROR) {
      std::cerr << "Error: " << resp.get_str() << "\n";
      ::close(fd);
      std::exit(1);
    }

    if (resp.get_type() != MessageType::OK) {
      std::cerr << "Error: unexpected response to LOGIN\n";
      ::close(fd);
      std::exit(1);
    }

    return fd;
  } catch (Exception &ex) {
    if (fd >= 0) ::close(fd);
    std::cerr << "Error: " << ex.what() << "\n";
    std::exit(1);
  }
}

