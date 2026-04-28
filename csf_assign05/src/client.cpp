#include <iostream>
#include <cassert>
#include "message.h"
#include "wire.h"
#include "io.h"
#include "except.h"
#include "server.h"
#include "passwd_db.h"
#include "client.h"

Client::Client(int fd, Server *server)
  : m_fd(fd)
  , m_server(server) 
  , m_mode(ClientMode::INVALID) {
}

Client::~Client() {
  //unregister if DISPLAY to prevent memory leak
  if (m_mode == ClientMode::DISPLAY) {
    m_server->unregister_display(this);
  }
  if (m_fd >= 0) {
    ::close(m_fd);
  }
}

//send message to client
void Client::send_msg(const Message &msg) {
  std::string out;
  Wire::encode(msg, out);
  IO::send(out, m_fd);
}

//try login
void Client::login() {
  std::string raw;
  IO::receive(m_fd, raw);
  Message req;
  Wire::decode(raw, req);

  // Check if the message is a login request
  if (req.get_type() != MessageType::LOGIN) {
    throw ProtocolError("Not Login Message");
  }

  // Authenticate the user
  if (!PasswordDB::authenticate(req.get_str())) {
    Message err(MessageType::ERROR, "Bad Login");
    send_msg(err);
    throw ProtocolError("auth error");
  }

  // Set the client mode
  m_mode = req.get_client_mode();
  if (m_mode != ClientMode::UPDATER && m_mode != ClientMode::DISPLAY) {
    Message err(MessageType::ERROR, "invalid client mode");
    send_msg(err);
    throw ProtocolError("invalid client mode");
  }

  //if all passed and excuted show success
  Message ok(MessageType::OK, "successful login");
  send_msg(ok);
}

// Handle chat messages from the client.
void Client::chat() {
  //login
  login();

  //update or display
  if (m_mode == ClientMode::UPDATER) {
    chat_updater();
  } else {
    chat_display();
  }
}

//update chat
void Client::update_chat() {
  while (true) {
    std::string raw;
    IO::receive(m_fd, raw);
    Message req;
    Wire::decode(raw, req);

    try {
      //check what request type
      switch (req.get_type()) {
        case MessageType::QUIT: {
          send_msg(Message(MessageType::OK, "goodbye"));
          return;
        }
        //if its a new order make sure id is 1 then add to system
        case MessageType::ORDER_NEW: {
          if (!req.has_order()) {
            throw SemanticError("ORDER_NEW missing order");
          }

          if (req.get_order()->get_id() != 1) {
            throw SemanticError("ORDER_NEW must have order id of 1");
          }
          int new_id = m_server->add_order(req.get_order()->duplicate());
          send_msg(Message(MessageType::OK,
            "Created order id " + std::to_string(new_id)));
          break;
        }

        //check if item update is valid
        case MessageType::ITEM_UPDATE: {
          m_server->update_item(req.get_order_id(),
                                req.get_item_id(),
                                req.get_item_status());
          send_msg(Message(MessageType::OK, "successful item update"));
          break;
        }

        //check if order update is valid
        case MessageType::ORDER_UPDATE: {
          m_server->update_order(req.get_order_id(),
                                 req.get_order_status());
          send_msg(Message(MessageType::OK, "successful order update"));
          break;
        }
        //check for other message types
        default: {

          throw ProtocolError("unexpected message type from updater");
        }
      }
    } catch (SemanticError &ex) {
      send_msg(Message(MessageType::ERROR, ex.what()));

    }
  }
}


//display to user
void Client::chat_display() {
  m_server->register_display(this);

  auto orders = m_server->snapshot_orders();
  for (auto &order : orders) {
    Message m(MessageType::DISP_ORDER, order);
    send_msg(m);
  }

  while (true) {
    auto msg = m_queue.dequeue();
    if (msg) {
      send_msg(*msg);
    } else {
      send_msg(Message(MessageType::DISP_HEARTBEAT));
    }
  }
}

