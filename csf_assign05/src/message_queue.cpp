#include <iostream>
#include <ctime>
#include "guard.h"
#include "message_queue.h"

MessageQueue::MessageQueue() {
  // Initialize mutex and semaphore
  pthread_mutex_init(&m_lock, nullptr);
  sem_init(&m_sem, 0, 0);
}

MessageQueue::~MessageQueue() {
  // Clean up mutex and semaphore
  pthread_mutex_destroy(&m_lock);
  sem_destroy(&m_sem);
}

void MessageQueue::enqueue(std::shared_ptr<Message> msg) {
  //lock the mutex
  pthread_mutex_lock(&m_lock);
  m_queue.push_back(msg);
  pthread_mutex_unlock(&m_lock);

  //message is available
  sem_post(&m_sem);
}

std::shared_ptr<Message> MessageQueue::dequeue() {
  //1 second wait
  std::timespec ts;
  std::timespec_get(&ts, TIME_UTC);
  ts.tv_sec += 1;

  //Wait at most 1 second
  if (sem_timedwait(&m_sem, &ts) < 0) {
    //timeout
    return std::shared_ptr<Message>();
  }

  //pop after message recieved
  pthread_mutex_lock(&m_lock);
  std::shared_ptr<Message> msg = m_queue.front();
  m_queue.pop_front();
  pthread_mutex_unlock(&m_lock);
  return msg;
}


