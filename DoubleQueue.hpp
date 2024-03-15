#ifndef DOUBLEQUEUE_HPP_
#define DOUBLEQUEUE_HPP_

#include <pthread.h>
#include <optional>

///////////////////////////////////////////////////////////////////////////////
// A DoubleQueue is a class that represents a queue of double values
//
// The queue supports:
// - adding doubles to the end of the queue
// - removing doubles from the end of the queue
// - removing a double from the end of the queue and waiting
//   for a double to be added if there isn't one already.
// The queue is thread safe, with no potential for data races, or deadlocks
///////////////////////////////////////////////////////////////////////////////

class DoubleQueue {
 public:
  // Constructor for a DoubleQueue.
  // Initializes the queue to be empty
  // and ready to handle concurrent operations
  DoubleQueue();

  // Destructor for DoubleQueue.
  // Cleans up any remaining elements in the queue
  // and any synronization methods used for maintaining
  // the state of the queue.
  ~DoubleQueue();

  // Adds a double to the end of the queue
  // This operation is thread safe.
  //
  // Arguments:
  // - val: the double value to add to the end of the queue
  //
  // Returns:
  // - true if the operation is successful
  // - false if the queue is closed
  bool add(double val);

  // Closes the queue.
  //
  // Any calls to add() that happens after calling close should fail
  // and return false.
  //
  // calls to remove() or wait_remove() should return nullopt
  // if there are no elements in the queue left.
  //
  // Threads blocked on wait_remove() will be waken up to either
  // process any values left in the queue or return nullopt
  void close();

  // Removes a double from the front of the queue
  // This operation is thread safe.
  //
  // Arguments:
  // - ret: An output parameter to return the value at the front of the list
  //
  // Returns:
  // - True if a value was successfully removed
  // - False if there were no values in the queue
  std::optional<double> remove();

  // Removes a double from the front of the queue but if there is no double
  // in the queue, calling thread will block or spin until there
  // is a double available. If the the queue is closed and the queue
  // is empty, then it returns nullopt instead.
  //
  // This operation is thread safe.
  //
  // Arguments: None
  //
  // Returns:
  // - The value removed from the front of the queue
  // - nullopt if the queue is closed and empty
  std::optional<double> wait_remove();

  // Returns the length of the queue currently
  // This operation is thread safe.
  //
  // Arguments: None
  //
  // Returns:
  // The value length of (i.e. number of elements in) the queue
  int length();

  // Feel free to ignore these
  DoubleQueue(const DoubleQueue& other) = delete;
  DoubleQueue& operator=(const DoubleQueue& other) = delete;
  DoubleQueue(DoubleQueue&& other) = delete;
  DoubleQueue& operator=(DoubleQueue&& other) = delete;

 private:
  // define a new internal type used to represent
  // nodes in the Queue
  // Queue can be implemented as a linked list
  struct QueueNode {
    QueueNode* next;
    double value;
    QueueNode(double val) : value(val), next(nullptr) {}
  };

  // Fields
  QueueNode* head;
  QueueNode* tail;
  int size;
  bool isClosed;
  pthread_mutex_t mutex;
  pthread_cond_t cond;

  // Helper methods
  void lockQueue();
  void unlockQueue();
  void waitForItems();
  void notifyWaiters();
};

#endif  // DOUBLEQUEUE_HPP_
