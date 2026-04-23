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
// TODO: implement helper functions/clases as needed

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <hostname> <port>\n";
    return 1;
  }

  // TODO: implement
}
