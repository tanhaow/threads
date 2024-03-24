/* A simple multithreaded program:
    one thread blocks/waits to read doubles from user
    while another thread will receive those values and print */

#include "DoubleQueue.hpp"
#include <iostream>
#include <optional>
#include <stdexcept>

using namespace std;

DoubleQueue::DoubleQueue()
    : head(nullptr), tail(nullptr), size(0), isClosed(false) {
  pthread_mutex_init(&mutex, nullptr);
  pthread_cond_init(&cond, nullptr);
}

DoubleQueue::~DoubleQueue() {
  while (head != nullptr) {
    QueueNode* temp = head;
    head = head->next;
    delete temp;
  }
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond);
}

void DoubleQueue::lockQueue() {
  pthread_mutex_lock(&mutex);
}

void DoubleQueue::unlockQueue() {
  pthread_mutex_unlock(&mutex);
}

void DoubleQueue::waitForItems() {
  pthread_cond_wait(&cond, &mutex);
}

void DoubleQueue::notifyWaiters() {
  pthread_cond_broadcast(&cond);
}

// Adds a double to the end of the queue
bool DoubleQueue::add(double val) {
  lockQueue();
  if (isClosed) {
    unlockQueue();
    return false;
  }
  QueueNode* node = new QueueNode(val);
  if (tail == nullptr) {  // If queue is empty
    head = tail = node;   // The new node is now both head and tail
  } else {
    tail->next = node;  // If not empty, append the new node at the end
    tail = node;        // Update the tail pointer
  }
  size++;
  notifyWaiters();
  unlockQueue();
  return true;
}

// Closes the queue.
void DoubleQueue::close() {
  lockQueue();
  isClosed = true;
  notifyWaiters();
  unlockQueue();
}

// Removes a double from the front of the queue
optional<double> DoubleQueue::remove() {
  lockQueue();
  auto result = coreRemove();
  unlockQueue();
  return result;
}

// Removes a double from the front of the queue but if there is no double
// in the queue, calling thread will block or spin until there
// is a double available. If the the queue is closed and the queue
// is empty, then it returns nullopt instead.
optional<double> DoubleQueue::wait_remove() {
  lockQueue();
  while (size == 0 && !isClosed) {
    waitForItems();  // Wait for items if the queue is empty and not closed.
  }
  optional<double> value = nullopt;
  if (!(isClosed && size == 0)) {
    value = coreRemove();  // Use coreRemove here.
  }
  unlockQueue();
  return value;
}

// Private method: Assumes the mutex is already locked
optional<double> DoubleQueue::coreRemove() {
  // If queue is empty, then nothing to remove
  if (size == 0) {
    return nullopt;
  }
  // else, remove a double from the front of the queue
  QueueNode* temp = head;      // Hold the current head
  double value = temp->value;  // Extract its value
  head = head->next;           // Move head to the next node
  if (head == nullptr) {
    tail = nullptr;  // If the queue is now empty, tail is also null
  }
  delete temp;   // Free the removed node
  size--;        // Decrease the queue size
  return value;  // Return the value of the removed node
}

// Returns the current length of the queue
int DoubleQueue::length() {
  lockQueue();
  int currentSize = size;
  unlockQueue();
  return currentSize;
}
