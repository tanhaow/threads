#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "qdbmp.hpp"

using namespace std;

constexpr UCHAR MAX_COLOR_VALUE = 255U;
unsigned int height;
unsigned int width;
mutex blurMutex;

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

  // Load image and prepare for processing
  BitMap image(input_fname);
  if (image.check_error() != BMP_OK) {
    perror("ERROR: Failed to open BMP file.");
    return EXIT_FAILURE;
  }

  height = image.height();
  width = image.width();
  BitMap blur(width, height);
  if (blur.check_error() != BMP_OK) {
    perror("ERROR: Failed to open BMP file.");
    return EXIT_FAILURE;
  }

  // Calculate workload per thread (by rows)
  int rowsPerThread = height / thread_count;
  int extraRows = height % thread_count;

  vector<thread> threads;

  int currentStartY = 0;
  for (int i = 0; i < thread_count; ++i) {
    int rowsForThisThread = rowsPerThread + (i < extraRows ? 1 : 0);
    int endY = currentStartY + rowsForThisThread - 1;

    // Log the section of the image assigned to each thread
    cout << "Thread " << i + 1 << " processing rows " << currentStartY << " to "
         << endY << endl;

    // Spawn a thread to process a section of the image
    threads.emplace_back(processSection, 0, currentStartY, width - 1, endY,
                         ref(image), ref(blur), block_size);

    currentStartY += rowsForThisThread;
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
  // Iterate over each row in the section assigned to this thread
  for (int y = startY; y <= endY; ++y) {
    // startX and endX now represent the full width of the image for each row
    for (int x = startX; x <= endX; ++x) {
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

      // Avoid division by zero
      if (pixels_counter == 0)
        continue;

      // Calculate the average color of the block
      RGB average_color = {static_cast<UCHAR>(total_red / pixels_counter),
                           static_cast<UCHAR>(total_green / pixels_counter),
                           static_cast<UCHAR>(total_blue / pixels_counter)};

      // Set the pixel color on the blur image
      blur.set_pixel(x, y, average_color);
    }
  }
}
