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