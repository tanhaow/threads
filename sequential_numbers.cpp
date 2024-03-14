#include <iostream>
#include <iomanip>
#include <limits>
#include <ios>
#include <array>

using std::cout;
using std::endl;
using std::fixed;
using std::cin;
using std::setprecision;
using std::numeric_limits;
using std::streamsize;
using std::array;

// globals
//
// NOTE: When you modify this to use DobuleQueue to communciate between threads
// one thread will need this array, and the threads should use DoubleQueue to communicate
array<double, 6> nums;
int len = 0;


// reads the global array nums
// and prints out statistics on it
// if there are more than 5 elements in the
// array, then the array is truncated down to
// 5 elements
// The array should never exceed 6 elements
void print_nums();


// Reads a number from the console
// and adds it to the global array called "nums".
//
// returns false when the EOF (ctrl + d)
// is read. Meaning that the overall
// program should now terminate..
// returns true if the reading was successful
bool read_nums();

int main() {
  while (read_nums()) {
    print_nums();
  }

  return EXIT_SUCCESS;
}

void print_nums() {
  if (len <= 0) {
    return;
  }

  if (len > 5) {
    for (int i = 0; i < 5; i++) {
      nums.at(i) = nums.at(i+1);
    }
    len--;
  }

  double min = nums[0];
  double max = nums[0];
  double sum = nums[0];
  for (int i = 1; i < len; i++) {
    sum += nums.at(i);
    if (min > nums.at(i)) {
      min = nums.at(i);
    }
    if (max < nums.at(i)) {
      max = nums.at(i);
    }
  }
  double avg = sum / len;

  cout << setprecision(2) << std::fixed;
  cout << "Max: " << max << endl;
  cout << "Min: " << min << endl;
  cout << "Average: " << avg << endl;
  cout << "Last five: ";

  for ( int i = 0; i < len; i++) {
    cout << nums.at(i) << " ";
  }
  cout << endl;
}
 
bool read_nums() {
  double value;
  cin >> value;
  while (!cin) {
    // if an input that was passed in to cause reading to fail
    // then check for EOF, otherwise ignore a charcter of input
    // and try again
    if (cin.eof()) {
      return false;
    }

    cin.clear(); // clear error flags

    // ignore error unput that would cause the error.
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // try reading value again
    cin >> value;
  }

  nums.at(len) = value;
  len++;
  return true;
}
