README for Assignment 5

Names of team members, contributions:
James To: Updater and Display
Annabelle Li-Kroeger: Wire and IO
MS 2:
    James To: client and message_queue
    Annabelle Li-Kroeger: Server and README

Synchronization report (for Milestone 2):
Throughout the code, there are multiple orders and items that can be shared, accessed, and mutated across multiple clients. 
Specifically, the following server variables are shared between threads:

- std::unordered_set<Client *> my_clients;
- std::unordered_map<int, std::shared_ptr<Order>> my_orders;
- int next_order_id;

Guards and pthreads are used to ensure that only one thread can be accessed at any point in the code. The pthreads allow 
one mutex (my_lock) to synchronize these variables across the client-handling threads. This mutex is locked using RAII's 
Guard class, making sure that orders are mutated by only one thread at a time. This is specifically used in the Server 
functions:

- create_order
- update_item
- update_order
- add_client / remove_client
- send_order_message

These are accessible by the Client threads, and thus the shared server variables must be maintained by the guards. This makes 
race conditions impossible because the guard prevents a second thread from mutating the shared variables until after the first 
thread is done and the guard is unlocked. The variables must be synchronized because they are all shared by the server.
MessageQueue also utilizes its own separate mutex (m_lock) and a semaphore (m_sem) to ensure that its internal deque:

- std::deque<std::shared_ptr<Message>> m_queue;

is only able to be altered and accessed by one thread at a time. The semaphore is a separate synchronization primitive:
sem_post and sem_timedwait are thread-safe atomic operations and do not require mutex protection. The semaphore count tracks 
how many messages are available, allowing the display client thread to wait up to one second for a new message before sending 
a heartbeat instead. Each MessageQueue is owned by a single Client object, and its mutex ensures safe concurrent access between
the display client thread (which calls dequeue) and any server thread that calls enqueue during a broadcast.

We know that there are no deadlocks because there are two separate mutexes (my_lock in Server and m_lock in each MessageQueue) 
and they are always acquired in the same order. Specifically, broadcast() is called while my_lock is still held, and it calls 
enqueue(), which then acquires m_lock. No code path ever acquires my_lock while m_lock is already held. Since the locks are 
always acquired in the same order (Server → MessageQueue), circular waiting is impossible and the system is deadlock-free.