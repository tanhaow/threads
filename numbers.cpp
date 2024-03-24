#include <pthread.h>
#include <unistd.h>   // For sleep
#include <algorithm>  // For max_element and min_element
#include <iomanip>
#include <iostream>
#include <numeric>
#include <optional>
#include <sstream>  // For std::stringstream
#include <vector>
#include "DoubleQueue.hpp"  // Ensure this is implemented correctly.

using namespace std;

DoubleQueue queue;
volatile bool endOfInput = false;  // Flag to indicate the end of input (EOF)

pthread_mutex_t cinMutex = PTHREAD_MUTEX_INITIALIZER;   // Mutex for cin
pthread_mutex_t coutMutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex for cout

// Function to perform synchronized input operation
bool synchronizedInput(string& input) {
  pthread_mutex_lock(&cinMutex);
  if (!getline(cin, input)) {
    endOfInput = true;  // Signal EOF has been encountered
    pthread_mutex_unlock(&cinMutex);
    return false;
  }
  pthread_mutex_unlock(&cinMutex);
  return true;
}

// Function to perform synchronized output operation
void synchronizedOutput(const string& output) {
  pthread_mutex_lock(&coutMutex);  // Lock cout mutex
  cout << output;
  pthread_mutex_unlock(&coutMutex);  // Unlock cout mutex
}

void* readerThread(void* arg) {
  double num;
  while (true) {
    string input;
    if (!synchronizedInput(
            input))  // Perform synchronized input and check for EOF
      break;         // Exit loop if end of input

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
  queue.close();  // Signal to the printerThread that there will be no more
                  // numbers
  return nullptr;
}

void* printerThread(void* arg) {
  vector<double> lastFiveNumbers;
  optional<double> num;
  while ((num = queue.wait_remove())) {
    lastFiveNumbers.push_back(*num);
    if (lastFiveNumbers.size() > 5) {
      lastFiveNumbers.erase(lastFiveNumbers.begin());
    }

    stringstream output;  // Use stringstream to construct output
    output << setprecision(2) << fixed;
    output << "Max: "
           << *max_element(lastFiveNumbers.begin(), lastFiveNumbers.end())
           << endl;
    output << "Min: "
           << *min_element(lastFiveNumbers.begin(), lastFiveNumbers.end())
           << endl;
    output << "Average: "
           << accumulate(lastFiveNumbers.begin(), lastFiveNumbers.end(), 0.0) /
                  lastFiveNumbers.size()
           << endl;
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
