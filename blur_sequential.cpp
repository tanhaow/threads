#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include "qdbmp.hpp"

using std::cerr;
using std::cout;
using std::endl;
using std::max;
using std::min;
using std::string;

constexpr UCHAR MAX_COLOR_VALUE = 255U;
unsigned int height;
unsigned int width;

bool isPixelInBounds(size_t x, size_t y);

int main(int argc, char* argv[]) {
  // Check input commands
  if (argc != 4) {
    cerr << "Usage: " << argv[0] << " <input file> <output_file> <block_size>"
         << endl;
    return EXIT_FAILURE;
  }

  string input_fname{argv[1]};
  string output_fname{argv[2]};
  string block_size_str(argv[3]);
  int block_size;

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
    cerr << "The argument is not an integer." << endl;
    return EXIT_FAILURE;
  } catch (const std::out_of_range& e) {
    cerr << "The argument is out of integer range." << endl;
    return EXIT_FAILURE;
  }
  // If reach here, all input argv are valid.
  // cout << "The block size is: " << block_size << endl;

  // Construct a BitMap object using the input file specified
  BitMap image(input_fname);

  // Check the command above succeed
  if (image.check_error() != BMP_OK) {
    perror("ERROR: Failed to open BMP file.");
    return EXIT_FAILURE;
  }

  // Create a new BitMap for output the blur image
  height = image.height();
  width = image.width();
  BitMap blur(width, height);

  // Check the command above succeed
  if (image.check_error() != BMP_OK) {
    perror("ERROR: Failed to open BMP file.");
    return EXIT_FAILURE;
  }

  // Loop through each pixel and calcute its block average
  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; ++x) {
      size_t pixels_counter = 0;
      unsigned int total_red = 0, total_green = 0, total_blue = 0;

      size_t startY = max(0, static_cast<int>(y) - block_size);
      size_t endY = min(y + block_size, static_cast<size_t>(height - 1));
      size_t startX = max(0, static_cast<int>(x) - block_size);
      size_t endX = min(x + block_size, static_cast<size_t>(width - 1));

      for (size_t yyy = startY; yyy <= endY; ++yyy) {
        for (size_t xxx = startX; xxx <= endX; ++xxx) {
          // Assuming isPixelInBounds ensures xxx and yyy are within image
          // dimensions
          ++pixels_counter;
          RGB color = image.get_pixel(xxx, yyy);
          total_red += color.red;
          total_green += color.green;
          total_blue += color.blue;
        }
      }

      if (pixels_counter == 0) {
        continue;
      }

      // Debug output - casting to ensure numerical output
      cout << "Accumulated Colors for (" << x << ", " << y
           << "): Red=" << static_cast<unsigned int>(total_red)
           << ", Green=" << static_cast<unsigned int>(total_green)
           << ", Blue=" << static_cast<unsigned int>(total_blue)
           << ", Counter=" << pixels_counter << endl;

      RGB average_color = {static_cast<UCHAR>(total_red / pixels_counter),
                           static_cast<UCHAR>(total_green / pixels_counter),
                           static_cast<UCHAR>(total_blue / pixels_counter)};
      blur.set_pixel(x, y, average_color);
    }
  }

  // Output the negative image to disk
  blur.write_file(output_fname);

  if (image.check_error() != BMP_OK) {
    perror("ERROR: Failed to open BMP file.");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

bool isPixelInBounds(size_t x, size_t y) {
  // Check if the pixel coordinates are within the image bounds
  if (x < width && y < height) {
    cout << "Pixel (" << x << ", " << y << ") is in bounds." << endl;
    return true;  // The pixel is within bounds
  } else {
    cout << "Pixel (" << x << ", " << y << ") is out of bounds." << endl;
    return false;  // The pixel is out of bounds
  }
}