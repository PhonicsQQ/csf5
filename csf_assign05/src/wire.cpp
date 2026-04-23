#include <string>
#include <unordered_map>
#include <cassert>
#include "util.h"
#include "model.h"
#include "except.h"
#include "wire.h"

namespace {

// Private data and helper functions

const std::unordered_map<ClientMode, std::string> s_client_mode_to_str = {
  { ClientMode::INVALID, "INVALID" },
  { ClientMode::UPDATER, "UPDATER" },
  { ClientMode::DISPLAY, "DISPLAY" },
};

const std::vector<std::pair<std::string, ClientMode>> s_str_to_client_mode_vec =
  Util::invert_unordered_map(s_client_mode_to_str);

const std::unordered_map<std::string, ClientMode> s_str_to_client_mode(
  s_str_to_client_mode_vec.begin(),
  s_str_to_client_mode_vec.end()
);

const std::unordered_map<MessageType, std::string> s_message_type_to_str = {
  { MessageType::INVALID, "INVALID" },
  { MessageType::LOGIN, "LOGIN" },
  { MessageType::QUIT, "QUIT" },
  { MessageType::ORDER_NEW, "ORDER_NEW" },
  { MessageType::ITEM_UPDATE, "ITEM_UPDATE" },
  { MessageType::ORDER_UPDATE, "ORDER_UPDATE" },
  { MessageType::OK, "OK" },
  { MessageType::ERROR, "ERROR" },
  { MessageType::DISP_ORDER, "DISP_ORDER" },
  { MessageType::DISP_ITEM_UPDATE, "DISP_ITEM_UPDATE" },
  { MessageType::DISP_ORDER_UPDATE, "DISP_ORDER_UPDATE" },
  { MessageType::DISP_HEARTBEAT, "DISP_HEARTBEAT" },
};

const std::vector<std::pair<std::string, MessageType>> s_str_to_message_type_vec =
  Util::invert_unordered_map(s_message_type_to_str);

const std::unordered_map<std::string, MessageType> s_str_to_message_type(
  s_str_to_message_type_vec.begin(),
  s_str_to_message_type_vec.end()
);

const std::unordered_map<ItemStatus, std::string> s_item_status_to_str = {
  { ItemStatus::INVALID, "INVALID" },
  { ItemStatus::NEW, "NEW" },
  { ItemStatus::IN_PROGRESS, "IN_PROGRESS" },
  { ItemStatus::DONE, "DONE" },
};

const std::vector<std::pair<std::string, ItemStatus>> s_str_to_item_status_vec =
  Util::invert_unordered_map(s_item_status_to_str);

const std::unordered_map<std::string, ItemStatus> s_str_to_item_status(
  s_str_to_item_status_vec.begin(),
  s_str_to_item_status_vec.end()
);

const std::unordered_map<OrderStatus, std::string> s_order_status_to_str = {
  { OrderStatus::INVALID, "INVALID" },
  { OrderStatus::NEW, "NEW" },
  { OrderStatus::IN_PROGRESS, "IN_PROGRESS" },
  { OrderStatus::DONE, "DONE" },
  { OrderStatus::DELIVERED, "DELIVERED" },
};

const std::vector<std::pair<std::string, OrderStatus>> s_str_to_order_status_vec =
  Util::invert_unordered_map(s_order_status_to_str);

const std::unordered_map<std::string, OrderStatus> s_str_to_order_status(
  s_str_to_order_status_vec.begin(),
  s_str_to_order_status_vec.end()
);

// take in order and turn it into message string
std::string encode_order(const std::shared_ptr<Order> &order) {
  if (!order || order->get_num_items() == 0) {
    throw InvalidMessage("Order cannot be empty");
  }
  // create string to store formatted order
  std::string s;
  s += std::to_string(order->get_id()) + "," + Wire::order_status_to_str(order->get_status()) + ",";

  // format each item in order
  for (int i = 0; i < order->get_num_items(); ++i) {
    // while this order is not the last order
    if (i > 0) {
      s += ";";
    }
    auto item = order->at(i);
    s += std::to_string(item->get_order_id()) + ":" + std::to_string(item->get_id()) + ":" + Wire::item_status_to_str(item->get_status());
    s += ":" + item->get_desc() + ":" + std::to_string(item->get_qty());
  }

  return s;
}

// ensure that order message is valid
int validate_id_message(const std::string &s, int min) {
 if (s.empty()) {
    throw InvalidMessage("message is empty");
 }
  for (char c : s) {
    if (c < '0' || c > '9') {
      throw InvalidMessage("invalid character detected");
    }
  }
  try {
    int current = std::stoi(s);
    if (current < min) {
      throw InvalidMessage("out-of-range character detected");
    }
    return current;
  } 
  catch (std::out_of_range &) {
    throw InvalidMessage("out-of-range character detected");
  }
}

// take in message string and turn it into order
std::shared_ptr<Order> decode_order(const std::string &s) {
  
  // ensure that first substring in order (id) exists
  size_t id_delim = s.find(',');
  if (id_delim == std::string::npos) {
    throw InvalidMessage("Missing comma separating order id from order status");
  }

  // ensure that second substring in order (status) exists
  size_t status_delim = s.find(',', id_delim + 1);
  if (status_delim == std::string::npos) {
    throw InvalidMessage("Missing comma separating order status from order items");
  }

  // extract id, status, and items
  std::string id_str = s.substr(0, id_delim);
  std::string status_str = s.substr(id_delim + 1, status_delim - id_delim - 1);
  std::string items_str = s.substr(status_delim + 1);

  // check if order message is valid
  int my_id = validate_id_message(id_str, 1);
  OrderStatus my_status = Wire::str_to_order_status(status_str);
  if (my_status == OrderStatus::INVALID) {
    throw InvalidMessage("Order status invalid");
  }

  //validate items in message
  auto order = std::make_shared<Order>(my_id, my_status);
  if (items_str.empty()) {
    throw InvalidMessage("Cannot submit an empty order");
  }
  std::vector<std::string> item_strs = Util::split(items_str, ';');
  if (item_strs.empty()) {
    throw InvalidMessage("Cannot submit an empty order");
  }

  // go through each item and validate it
  for (const std::string &item_str : item_strs) {
    std::vector<std::string> item_message = Util::split(item_str, ':');
    if (item_message.size() != 5) {
      throw InvalidMessage("Item message format invalid");
    }

    // extract substrings from order message and validate them
    int current_order_id = validate_id_message(item_message[0], 1);
    int current_id = validate_id_message(item_message[1], 1);
    ItemStatus item_status = Wire::str_to_item_status(item_message[2]);
    if (item_status == ItemStatus::INVALID) {
      throw InvalidMessage("Item status not of valid format");
    }
    std::string item_description = item_message[3];
    int item_quantity = validate_id_message(item_message[4], 1);

    // create order
    order->add_item(std::make_shared<Item>(current_order_id, current_id, item_status, item_description, item_quantity));
  }

  return order;
}

} // end of anonymous namespace for helper functions

namespace Wire {

std::string client_mode_to_str(ClientMode client_mode) {
  auto i = s_client_mode_to_str.find(client_mode);
  return (i != s_client_mode_to_str.end()) ? i->second : "<invalid>";
}

ClientMode str_to_client_mode(const std::string &s) {
  auto i = s_str_to_client_mode.find(s);
  return (i != s_str_to_client_mode.end()) ? i->second : ClientMode::INVALID;
}

std::string message_type_to_str(MessageType message_type) {
  auto i = s_message_type_to_str.find(message_type);
  return (i != s_message_type_to_str.end()) ? i->second : "<invalid>";
}

MessageType str_to_message_type(const std::string &s) {
  auto i = s_str_to_message_type.find(s);
  return (i != s_str_to_message_type.end()) ? i->second : MessageType::INVALID;
}

std::string item_status_to_str(ItemStatus item_status) {
  auto i = s_item_status_to_str.find(item_status);
  return (i != s_item_status_to_str.end()) ? i->second : "<invalid>";
}

ItemStatus str_to_item_status(const std::string &s) {
  auto i = s_str_to_item_status.find(s);
  return (i != s_str_to_item_status.end()) ? i->second : ItemStatus::INVALID;
}

std::string order_status_to_str(OrderStatus order_status) {
  auto i = s_order_status_to_str.find(order_status);
  return (i != s_order_status_to_str.end()) ? i->second : "<invalid>";
}

OrderStatus str_to_order_status(const std::string &s) {
  auto i = s_str_to_order_status.find(s);
  return (i != s_str_to_order_status.end()) ? i->second : OrderStatus::INVALID;
}

void encode(const Message &msg, std::string &s) {
  // extract message
  s = message_type_to_str(msg.get_type());

  // encode message according to type
  switch (msg.get_type()) {
    
    case MessageType::DISP_HEARTBEAT:
      break;
    
    case MessageType::LOGIN:
      s += "|" + client_mode_to_str(msg.get_client_mode()) + "|" + msg.get_str();
      break;

    case MessageType::QUIT:
    case MessageType::OK:
    case MessageType::ERROR:
      s += "|" + msg.get_str();
      break;
    
    // orders
    case MessageType::ORDER_NEW:
    case MessageType::DISP_ORDER:
      s += "|" + encode_order(msg.get_order());
      break;
    
    // item updates
    case MessageType::ITEM_UPDATE:
    case MessageType::DISP_ITEM_UPDATE:
      s += "|" + std::to_string(msg.get_order_id()) + "|" + std::to_string(msg.get_item_id()) + "|" + item_status_to_str(msg.get_item_status());
      break;

    // order updates
    case MessageType::ORDER_UPDATE:
    case MessageType::DISP_ORDER_UPDATE:
      // type|order_id|order_status
      s += "|" + std::to_string(msg.get_order_id()) + "|" + order_status_to_str(msg.get_order_status());
      break;
    
    default:
      throw InvalidMessage("Message type was not recognized");
  }
}

void decode(const std::string &s, Message &msg) {

  // extract substrings from message to decode them
  std::vector<std::string> message_substrings = Util::split(s, '|');
  if (message_substrings.empty()) {
    throw InvalidMessage("empty message string");
  }

  // handle invalid messages
  MessageType message_type = str_to_message_type(message_substrings[0]);
  if (message_type == MessageType::INVALID) {
    throw InvalidMessage("message type not recognized");
  }
  
  // decode message according to type
  switch (message_type) {

    // handle heartbeat message
    case MessageType::DISP_HEARTBEAT:
      if (message_substrings.size() != 1) {
        throw InvalidMessage("Invalid format for DISP_HEARTBEAT");
      }
      msg = Message(message_type);
      break;
    
    // handle login message
    case MessageType::LOGIN:
      if (message_substrings.size() != 3) {
        throw InvalidMessage("Invalid format for LOGIN message");
      }
      {
        ClientMode my_mode = str_to_client_mode(message_substrings[1]);
        if (my_mode == ClientMode::INVALID)
          throw InvalidMessage("Invalid client mode");
        msg = Message(message_type, my_mode, message_substrings[2]);
      }
      break;

    // handle status messages
    case MessageType::QUIT:
    case MessageType::OK:
    case MessageType::ERROR:
      if (message_substrings.size() != 2) {
        throw InvalidMessage("Invalid quit, ok, or error message");
      }
      msg = Message(message_type, message_substrings[1]);
      break;

    // handle orders
    case MessageType::ORDER_NEW:
    case MessageType::DISP_ORDER:
      if (message_substrings.size() != 2) {
        throw InvalidMessage("Invalid order message");
      }
      {
        auto order = decode_order(message_substrings[1]);
        msg = Message(message_type, order);
      }
      break;

    // handle items
    case MessageType::ITEM_UPDATE:
    case MessageType::DISP_ITEM_UPDATE:
      
      // handle invalid format
      if (message_substrings.size() != 4) {
        throw InvalidMessage("Invalid item message format");
      }

      // extract and validate item substrings
      {
        int order_id = validate_id_message(message_substrings[1], 1);
        int item_id = validate_id_message(message_substrings[2], 1);
        ItemStatus my_status = str_to_item_status(message_substrings[3]);
        if (my_status == ItemStatus::INVALID)
          throw InvalidMessage("Item status id not valid");
        msg = Message(message_type, order_id, item_id, my_status);
      }
      break;

    // handle order updates
    case MessageType::ORDER_UPDATE:
    case MessageType::DISP_ORDER_UPDATE:
      if (message_substrings.size() != 3) {
        throw InvalidMessage("Order update message format not valid");
      }
      {
        int order_id = validate_id_message(message_substrings[1], 1);
        OrderStatus order_status = str_to_order_status(message_substrings[2]);
        if (order_status == OrderStatus::INVALID) {
          throw InvalidMessage("Order status not valid");
        }
        msg = Message(message_type, order_id, order_status);
      }
      break;

    default:
      throw InvalidMessage("Message type is not recognized");
  }
}

}
