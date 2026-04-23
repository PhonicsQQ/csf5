// Main program for the updater client

#include <iostream>
#include <string>
#include <stdexcept>
#include "csapp.h"
#include "model.h"
#include "message.h"
#include "wire.h"
#include "io.h"
#include "except.h"
#include "util.h"
#include "client_util.h"



void result(const Message &response) {
  if (response.get_type() == MessageType::OK) {
    //successful response
    std::cout << "Success: " << response.get_str() << "\n";
    
  } else if (response.get_type() == MessageType::ERROR) {
    //error response
    std::cout << "Failure: " << response.get_str() << "\n";
  } else {
    throw ProtocolError("bad input");
  }
}

Message sendReceive(int fd, const Message &req) {
  std::string out;
  //send message after encode
  Wire::encode(req, out);   
  IO::send(out, fd);        

  std::string in;
  IO::receive(fd, in);     
  Message resp;
  //decode recieved message and return
  Wire::decode(in, resp);   
  return resp;
}


//see if line exist
std::string getline() {
  std::string line;
  if (!std::getline(std::cin, line)) {
    throw IOException("bad input");
  }
  return line;
}

int getInt() {
  std::string line = getline();
  try {
    return std::stoi(line);
  } catch (std::exception &) {
    //when given line is not an int / invalid
    throw IOException("not integer");
  }
}

void orderNew(int fd) {
  int num_items = getInt();

  //new order with arbitrary id
  auto order = std::make_shared<Order>(1, OrderStatus::NEW);
  for (int i = 0; i < num_items; ++i) {
    //get item id and item details
    std::string desc = getline();
    int qty = getInt();
    int item_id = getInt();

    //add item to order
    order->add_item(
      std::make_shared<Item>(1, item_id, ItemStatus::NEW, desc, qty));
  }

  Message req(MessageType::ORDER_NEW, order);
  Message resp = sendReceive(fd, req);
  result(resp);
}

void itemUpdate(int fd) {
  //item and order id
  int order_id = getInt();
  int item_id = getInt();
  std::string status_str = getline();

  //check status/if item is valid
  ItemStatus status = Wire::str_to_item_status(status_str);
  Message req(MessageType::ITEM_UPDATE, order_id, item_id, status);
  Message resp = sendReceive(fd, req);
  result(resp);
}

void orderUpdate(int fd) {
  //order id
  int order_id = getInt();
  std::string status_str = getline();

  //check status/if order is valid
  OrderStatus status = Wire::str_to_order_status(status_str);
  Message req(MessageType::ORDER_UPDATE, order_id, status);
  Message resp = sendReceive(fd, req);
  result(resp);
}

bool quit(int fd) {
  Message req(MessageType::QUIT, "quit");
  Message resp = sendReceive(fd, req);
  if (resp.get_type() != MessageType::OK) {
    throw ProtocolError("expected OK in response to QUIT");
  }
  return true;
}

void input(int fd) {
  while (true) {
    // Prompt must appear immediately
    std::cout << "> " << std::flush;  
    std::string cmd;
    //valid check
    if (!std::getline(std::cin, cmd)) {
      break; 
    }
    //check instruction
    if (cmd == "quit") {
      if (quit(fd))
        break;
    } else if (cmd == "order_new") {
      orderNew(fd);
    } else if (cmd == "item_update") {
      itemUpdate(fd);
    } else if (cmd == "order_update") {
      orderUpdate(fd);
    }
  }
}



int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <hostname> <port>\n";
    return 1;
  }
  int fd = connectLogin(argv[1], argv[2], ClientMode::UPDATER);

  try {
    input(fd);
  } catch (Exception &ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    ::close(fd);
    return 1;
  }

  ::close(fd);
  return 0;
  
}
