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

std::string encode_order(const std::shared_ptr<Order> &order) {
  if (!order || order->get_num_items() == 0) {
    throw InvalidMessage("Order cannot be empty");
  }
  // create string to store formatted order
  std::string s;
  s += std::to_string(order->get_id()) + "," + Wire::order_status_to_str(order->get_status()) + ",";

  // format each item in order
  for (int i = 0; i < order->get_num_items(); ++i) {
    if (i > 0) {
      s += ";";
    }
    auto item = order->at(i);
    s += std::to_string(item->get_order_id()) + ":" + std::to_string(item->get_id()) + ":" + Wire::item_status_to_str(item->get_status());
    s += ":" + item->get_desc() + ":" + std::to_string(item->get_qty());
  }

  return s;
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

    default:
      throw InvalidMessage("Message type is not recognized");
  }
}

}
