#include "ImageDimensions.h"

ImageDimensions::ImageDimensions() : rows(-1), cols(-1) {}

ImageDimensions::ImageDimensions(int rows, int cols) : rows(rows), cols(cols) {}

int ImageDimensions::area() const { return rows * cols; }

bool ImageDimensions::within(const ImageDimensions &dimensions) const {
  return row >= 0 && col >= 0 && row < dimensions.rows && col < dimensions.cols;
}

bool ImageDimensions::halfTheSizeOf(const ImageDimensions &dimensions) const {
  return rows == dimensions.rows / 2 && cols == dimensions.cols / 2;
}

bool operator==(const ImageDimensions &lhs, const ImageDimensions &rhs) {
  return lhs.rows == rhs.rows && lhs.cols == rhs.cols;
}
