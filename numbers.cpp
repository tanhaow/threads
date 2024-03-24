#include <pthread.h>
#include <unistd.h>   // For sleep
#include <algorithm>  // For max_element and min_element
#include <iomanip>
#include <iostream>
#include <numeric>
#include <optional>
#include <vector>
#include "DoubleQueue.hpp"

using namespace std;

DoubleQueue queue;
volatile bool endOfInput = false;  // Flag to indicate the end of input (EOF)

pthread_mutex_t cinMutex = PTHREAD_MUTEX_INITIALIZER;   // Mutex for cin
pthread_mutex_t coutMutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex for cout

// Function to perform synchronized input operation
string synchronizedInput() {
  string input;
  pthread_mutex_lock(&cinMutex);    // Lock cin mutex
  getline(cin, input);              // reads standard input (cin)
  pthread_mutex_unlock(&cinMutex);  // Unlock cin mutex
  return input;
}

// Function to perform synchronized output operation
void synchronizedOutput(const string& output) {
  pthread_mutex_lock(&coutMutex);  // Lock cout mutex
  cout << output;
  pthread_mutex_unlock(&coutMutex);  // Unlock cout mutex
}

void* readerThread(void* arg) {
  double num;
  while (!endOfInput) {
    string input = synchronizedInput();  // Perform synchronized input
    if (input.empty())
      continue;  // Skip empty input lines

    // convert string input to double, and add it to queue
    try {
      num = stod(input);
      queue.add(num);
      sleep(1);  // Sleep for 1 second after adding a number
    } catch (const invalid_argument& e) {
      // Ignore non-numeric inputs
    } catch (const out_of_range& e) {
      // Ignore inputs that are out of double range
    }
  }
  return nullptr;
}

void* printerThread(void* arg) {
  vector<double> lastFiveNumbers;
  optional<double> num;
  // loop as long as queue.wait_remove() returns a value:
  // If the queue is empty, blocks the thread
  // until a new item is added to the queue or the queue is closed.
  while ((num = queue.wait_remove())) {
    lastFiveNumbers.push_back(*num);
    if (lastFiveNumbers.size() > 5) {
      lastFiveNumbers.erase(lastFiveNumbers.begin());
    }

    // Calculate and display stats
    double maxVal =
        *max_element(lastFiveNumbers.begin(), lastFiveNumbers.end());
    double minVal =
        *min_element(lastFiveNumbers.begin(), lastFiveNumbers.end());
    double avg =
        accumulate(lastFiveNumbers.begin(), lastFiveNumbers.end(), 0.0) /
        lastFiveNumbers.size();

    // Construct the output string
    stringstream output;
    output << setprecision(2) << fixed;
    output << "Max: " << maxVal << endl;
    output << "Min: " << minVal << endl;
    output << "Average: " << avg << endl;
    output << "Last five: ";
    for (double val : lastFiveNumbers) {
      output << val << " ";
    }
    output << endl;

    synchronizedOutput(output.str());  // Perform synchronized output
  }
  return nullptr;
}

int main() {
  pthread_t readerThreadId, printerThreadId;

  // Create threads
  pthread_create(&readerThreadId, nullptr, readerThread, nullptr);
  pthread_create(&printerThreadId, nullptr, printerThread, nullptr);

  // Wait for threads to finish
  pthread_join(readerThreadId, nullptr);
  pthread_join(printerThreadId, nullptr);

  // Destroy mutexes
  pthread_mutex_destroy(&cinMutex);
  pthread_mutex_destroy(&coutMutex);

  return 0;
}
