#include <iostream>
#include <memory>
#include <cstring> // for strerror()
#include <cerrno>  // for errno
#include "csapp.h"
#include "except.h"
#include "model.h"
#include "message.h"
#include "wire.h"
#include "client.h"
#include "guard.h"
#include "server.h"

namespace {


void *create_thread(void *my_client) {
  
  // detach thread
  std::unique_ptr<Client> client(static_cast<Client *>(my_client));
  pthread_detach(pthread_self());

  // run chat and catch all exceptions
  try {
    client->chat();
  } catch (...) {}

  return nullptr;
}

}

Server::Server() {
  next_order_id = 1000;
  pthread_mutex_init(&my_lock, nullptr);
}

Server::~Server() {
  pthread_mutex_destroy(&my_lock);
}

void Server::server_loop(const char *port) {
  int server_fd = open_listenfd(port);
  if (server_fd < 0)
    throw IOException(std::string("open_listenfd failed: ") + std::strerror(errno));

  while (true) {

    // accept TCP connection from client
    struct sockaddr_storage client_address;
    socklen_t address_length = sizeof(client_address);
    int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr *>(&client_address), &address_length);

    // ensure that client file was correctly accepted. If not, try again.
    if (client_fd < 0) {
      continue;
    }

    // create a new instance of the Client class to manage the resources needed by a client thread
    Client *client = new Client(client_fd, this);

    // start a new thread to communicate with the client
    pthread_t my_thread;
    if (pthread_create(&my_thread, nullptr, create_thread, client) != 0) {
      // handle unsuccessful creation
      delete client;
    }
  }
}

int Server::create_order(std::shared_ptr<Order> order) {
  int order_id;
  {
    // ensure that only this thread is mutating order_id
    Guard my_guard(my_lock);
    order_id = next_order_id++;
 
    // Update order id
    order->set_id(order_id);
    for (int i = 0; i < order->get_num_items(); i++) {
      order->at(i)->set_order_id(order_id);
    }
    my_orders[order_id] = order;
 
    // Broadcast DISP_ORDER to all connected display clients.
    auto message = std::make_shared<Message>(MessageType::DISP_ORDER, order->duplicate());
    broadcast(message);
  }
 
  return order_id;
}

void Server::broadcast(std::shared_ptr<Message> message) {
  // enqueue message and add to client's queues
  for (Client *client : my_clients)
    client->get_queue().enqueue(message->duplicate());
}

void Server::update_item(int order_id, int item_id, ItemStatus new_status) {
  
  // ensure thread is locked
  Guard my_guard(my_lock);
 
  // check orders for order_id
  auto my_iter = my_orders.find(order_id);
  if (my_iter == my_orders.end())
    throw SemanticError("Order with order_id is not found");
 
  // check order for item_id
  auto &order = my_iter->second;
  auto item = order->find_item(item_id);
  if (!item)
    throw SemanticError("Item with item_id is not found");
 
  // Update order status
  ItemStatus cur = item->get_status();
  // handle valid status changes
  if (cur == ItemStatus::NEW && new_status == ItemStatus::IN_PROGRESS) {}
  else if (cur == ItemStatus::IN_PROGRESS && new_status == ItemStatus::DONE) {}
  // handle case when status does not change
  else if (cur == new_status) {
    throw SemanticError("Must enter a new status. Cannot update to item's current status");
  }
  // handle all other invalid status changes
  else {
    throw SemanticError("Status change is invalid");
  }
 
  item->set_status(new_status);
 
  // Broadcast to all display clients
  auto message = std::make_shared<Message>(MessageType::DISP_ITEM_UPDATE, order_id, item_id, new_status);
  broadcast(message);
 
  // Check if order status needs to be updateed
  OrderStatus current_status = order->get_status();
  OrderStatus new_order_status = current_status;
 
  // handle case when order should be updated from new to in progress
  if (current_status == OrderStatus::NEW &&new_status == ItemStatus::IN_PROGRESS) {
    new_order_status = OrderStatus::IN_PROGRESS;
  }
  // handle case when order should be updated to done
  else if (current_status == OrderStatus::IN_PROGRESS || current_status == OrderStatus::NEW) {
    // Check if all items are done
    bool completed = true;
    for (int i = 0; i < order->get_num_items(); ++i) {
      if (order->at(i)->get_status() != ItemStatus::DONE) {
        completed = false;
        break;
      }
    }
    if (completed)
      new_order_status = OrderStatus::DONE;
  }
 
  // if order status changed, display to all connected clients
  if (new_order_status != current_status) {
    order->set_status(new_order_status);
    auto message = std::make_shared<Message>(MessageType::DISP_ORDER_UPDATE, order_id, new_order_status);
    broadcast(message);
  }
}

void Server::update_order(int order_id, OrderStatus new_status) {
  // ensure threads are locked
  Guard my_guard(my_lock);
 
  // ensure that order id is valid
  auto my_iter = my_orders.find(order_id);
  if (my_iter == my_orders.end()) {
    throw SemanticError("Order with order_id not found");
  }

  // retreive order
  auto &order = my_iter->second;
  OrderStatus current = order->get_status();
 
  // ensure order update is valid (must be not done status to delivered)
  if (current != OrderStatus::DONE || new_status != OrderStatus::DELIVERED) {
    throw SemanticError("cannot transition from current status to new_status");
  }
 
  order->set_status(new_status);
 
  // broadcast message
  auto message = std::make_shared<Message>(MessageType::DISP_ORDER_UPDATE, order_id, new_status);
  broadcast(message);
 
  // Remove delivered order
  my_orders.erase(my_iter);
}