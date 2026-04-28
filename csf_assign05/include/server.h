#ifndef SERVER_H
#define SERVER_H

#include <unordered_map>
#include <unordered_set>
#include <pthread.h>
#include "util.h"
#include "model.h"

// Forward declarations
class Client;
class Message;

class Server {
private:
  // Private data
  std::unordered_set<Client *> my_clients;
  std::unordered_map<int, std::shared_ptr<Order>> my_orders;
  int next_order_id;

  // thread lock to ensure that next_order_id cannot be mutated by multiple threads at once
  pthread_mutex_t my_lock;

  // no value semantics
  NO_VALUE_SEMANTICS(Server);

public:
  Server();
  ~Server();

  // Create a server socket listening on specified port,
  // and accept connections from clients. This function does
  // not return.
  void server_loop(const char *port);

  // Create new order and broadcast MessageType::DISP_ORDER to all
  // connected display clients. Returns order id
  int create_order(std::shared_ptr<Order> order);

  // Update an item. Handles errors with SemanticErrors
  // Otherwise, broadcasts MessageType::DISP_ITEM_UPDATE to all connected display clients
  // This function does not return.
  void update_item(int order_id, int item_id, ItemStatus new_status);

  // Update an order. Handles errors with SemanticErrors.
  // Otherwise, broadcasts MessageType::DISP_ORDER_UPDATE to all connected display clients.
  // Removes all completed orders
  // This function does not return.
  void update_order(int order_id, OrderStatus new_status);

  // Add a display client to my_clients.
  // This function does not return
  void add_client(Client *client);

  // Remove a display client to my_clients.
  // This function does not return
  void remove_client(Client *client);

  // Send MessageType::DISP_ORDER when a new client connects
  void send_order_message(Client *client);

private:
  // CALLER MUST LOCK THREAD. Display message to all connected display clients.
  // This function does not return.
  void broadcast(std::shared_ptr<Message> message);
};

#endif // SERVER_H
