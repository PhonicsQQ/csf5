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

  // TODO: additional public member functions

private:
  // TODO: private member functions
};

#endif // SERVER_H
