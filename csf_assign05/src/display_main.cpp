// Main program for the display client

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

namespace {

// This terminal escape sequence should be written to
// std::cout before the display client redisplays the current
// order information.
const char CLEAR_SCREEN[] = "\x1b[2J\x1b[H";

}

void updateScreen(std::map<int, std::shared_ptr<Order>> &orders) {
  std::cout << CLEAR_SCREEN;
  for (auto it = orders.begin(); it != orders.end(); it++) {
    printOrder(it->second);
  }
  std::cout.flush();
}

void printOrder(std::shared_ptr<Order> order) {
  std::cout << "Order " << order->get_id() << ": " << Wire::order_status_to_str(order->get_status()) << "\n";
  for (int i = 0; i < order->get_num_items(); i++) {
    std::shared_ptr<Item> item = order->at(i);

    std::cout << "  Item " << item->get_id() << ": " << Wire::item_status_to_str(item->get_status()) << "\n";

    std::cout << "    " << item->get_desc() << ", Quantity "
              << item->get_qty() << "\n";
  }
}

void receive(int fd) {
  std::map<int, std::shared_ptr<Order>> orders;
  while (true) {
    std::string in;
    IO::receive(fd, in);
    Message msg;
    Wire::decode(in, msg);

    //check if running
    if (msg.get_type() == MessageType::DISP_HEARTBEAT)
      continue;

    //ORDER
    if (msg.get_type() == MessageType::DISP_ORDER) {
      if (!msg.has_order()) {
        throw ProtocolError("DISP_ORDER missing order");
      }
      orders[msg.get_order()->get_id()] = msg.get_order();

    //ITEM UPDATE
    } else if (msg.get_type() == MessageType::DISP_ITEM_UPDATE) {
      auto it = orders.find(msg.get_order_id());
      if (it != orders.end()) {
        if (auto item = it->second->find_item(msg.get_item_id())) {
          item->set_status(msg.get_item_status());
        }
      }
    //ORDER UPDATE
    } else if (msg.get_type() == MessageType::DISP_ORDER_UPDATE) {
      auto it = orders.find(msg.get_order_id());
      if (it != orders.end()) {
        if (msg.get_order_status() == OrderStatus::DELIVERED) {
          orders.erase(it);
        } else {
          it->second->set_status(msg.get_order_status());
        }
      }
    //INVALID RESPONSE
    } else {
      throw ProtocolError("invalid response");
    }

    updateScreen(orders);
  }
}

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <hostname> <port>\n";
    return 1;
  }

  //connect
  int fd = connectLogin(argv[1], argv[2], ClientMode::DISPLAY);
  std::cout << CLEAR_SCREEN << std::flush;

  try {
    //look for valid reads
    receive(fd);
  } catch (Exception &ex) {
    //invalid response
    std::cerr << "Error: " << ex.what() << "\n";
    ::close(fd);
    return 1;
  }

  ::close(fd);
  return 0;
}
