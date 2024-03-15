#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "qdbmp.hpp"

using std::cerr;
using std::cout;
using std::endl;
using std::max;
using std::min;
using std::string;
using std::thread;
using std::vector;

constexpr UCHAR MAX_COLOR_VALUE = 255U;
unsigned int height;
unsigned int width;

void processSection(int startX,
                    int startY,
                    int endX,
                    int endY,
                    BitMap& image,
                    BitMap& blur,
                    int blockSize);

int main(int argc, char* argv[]) {
  // Check input commands
  if (argc != 5) {
    cerr << "Usage: " << argv[0]
         << " <input file> <output_file> <block_size> <thread_count>" << endl;
    return EXIT_FAILURE;
  }

  string input_fname{argv[1]};
  string output_fname{argv[2]};
  string block_size_str(argv[3]);
  string thread_count_str(argv[4]);
  int block_size;
  int thread_count;

  // Check if input block_size if valid (not <= 0)
  try {
    size_t pos;  // to indicate the position of the first character that cannot
                 // be converted
    block_size = stoi(block_size_str, &pos);
    // check whether the entire string was successfully converted to an integer
    if (pos != block_size_str.length()) {
      cerr << "The input block size is not an integer." << endl;
      return EXIT_FAILURE;
    }
    // check if the converted int is > 0
    if (block_size <= 0) {
      cerr << "The input block size should be larger than 0." << endl;
      return EXIT_FAILURE;
    }
  } catch (const std::invalid_argument& e) {
    cerr << "The input block size is not an integer." << endl;
    return EXIT_FAILURE;
  } catch (const std::out_of_range& e) {
    cerr << "The input block size is out of integer range." << endl;
    return EXIT_FAILURE;
  }

  // Check if input thread_count if valid
  try {
    size_t pos;
    thread_count = stoi(thread_count_str, &pos);
    if (pos != thread_count_str.length()) {
      cerr << "The input threads count is not an integer." << endl;
      return EXIT_FAILURE;
    }
    if (thread_count <= 0) {
      cerr << "The input thread count should be larger than 0." << endl;
      return EXIT_FAILURE;
    }
  } catch (const std::invalid_argument& e) {
    cerr << "The input threads count is not an integer." << endl;
    return EXIT_FAILURE;
  } catch (const std::out_of_range& e) {
    cerr << "The input threads count is out of integer range." << endl;
    return EXIT_FAILURE;
  }
  // If reach here, all input argv are valid.
  // cout << "The block size is: " << block_size << endl;

  // Construct a BitMap object using the input file specified
  BitMap image(input_fname);
  if (image.check_error() != BMP_OK) {
    perror("ERROR: Failed to open BMP file.");
    return EXIT_FAILURE;
  }

  // Create a new BitMap for output the blur image
  height = image.height();
  width = image.width();
  BitMap blur(width, height);
  // Check the command above succeed
  if (blur.check_error() != BMP_OK) {
    perror("ERROR: Failed to open BMP file.");
    return EXIT_FAILURE;
  }

  // Calculate workload per thread
  size_t totalPixels = width * height;
  size_t pixelsPerThread = totalPixels / thread_count;
  vector<thread> threads;

  // STEP 1: Calculate the work (pixels) to process for a thread
  for (int i = 0; i < thread_count; ++i) {
    int startX = (i * pixelsPerThread) % width;  // which column
    int startY = (i * pixelsPerThread) / width;  // which row
    int endPixel = min((i + 1) * pixelsPerThread, totalPixels);
    int endX = endPixel % width;
    int endY = endPixel / width;
    // Adjust for the case where endPixel is exactly at the end of a row
    // this is due to % mod operation effectively "wraps around" to the
    // beginning of the next row. but this is not what we want for determining
    // end boundaries.
    if (endX == 0) {
      endX = width;
      endY -= 1;
    }
    // STEP 2: Create the new thread to process these pixels
    thread newThread(processSection, startX, startY, endX, endY,
                     std::ref(image), std::ref(blur), block_size);
    // std::ref allows us to pass in a reference to access the original images
    // directly, without passing in a new copy of the image

    // STEP 3: Push this thread to our list of threads
    threads.push_back(std::move(newThread));
  }

  // Wait for all threads to complete
  for (auto& th : threads) {
    th.join();
  }

  // Output the blurred image to disk
  blur.write_file(output_fname);
  if (blur.check_error() != BMP_OK) {
    perror("ERROR: Failed to write BMP file.");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

// Process a section of the image within the specified bounds: from (startX,
// startY) to (endX, endY) Loop through each pixel and calcute its block average
void processSection(int startX,
                    int startY,
                    int endX,
                    int endY,
                    BitMap& image,
                    BitMap& blur,
                    int block_size) {
  // Iterate over each row in the section
  for (int y = startY; y <= endY; ++y) {
    // Determine the start and end x coordinates for the current row
    int currentStartX = (y == startY) ? startX : 0;
    int currentEndX = (y == endY) ? endX : width - 1;

    // Iterate over each pixel in the current row
    for (int x = currentStartX; x <= currentEndX; ++x) {
      size_t pixels_counter = 0;
      unsigned int total_red = 0, total_green = 0, total_blue = 0;

      // Calculate the neighborhood boundaries considering the block size
      int neighborStartY = max(0, y - block_size);
      int neighborEndY = min(y + block_size, static_cast<int>(height) - 1);
      int neighborStartX = max(0, x - block_size);
      int neighborEndX = min(x + block_size, static_cast<int>(width) - 1);

      // Sum up the color values of all neighboring pixels
      for (int yy = neighborStartY; yy <= neighborEndY; ++yy) {
        for (int xx = neighborStartX; xx <= neighborEndX; ++xx) {
          RGB color = image.get_pixel(xx, yy);
          total_red += color.red;
          total_green += color.green;
          total_blue += color.blue;
          ++pixels_counter;
        }
      }

      if (pixels_counter == 0) {
        continue;  // To avoid division by zero
      }

      RGB average_color = {static_cast<UCHAR>(total_red / pixels_counter),
                           static_cast<UCHAR>(total_green / pixels_counter),
                           static_cast<UCHAR>(total_blue / pixels_counter)};
      blur.set_pixel(x, y, average_color);
    }
  }
}
