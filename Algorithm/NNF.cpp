#include "NNF.h"

#include <QtGlobal>

NNF::NNF(ImageDimensions sourceDimensions, ImageDimensions targetDimensions)
    : sourceDimensions(sourceDimensions), targetDimensions(targetDimensions) {
  mappings = std::make_unique<ImageCoordinates[]>(sourceDimensions.area());
}

std::unique_ptr<ImageCoordinates[]> &NNF::getMappings() { return mappings; }

ImageCoordinates NNF::getMapping(const ImageCoordinates &from) const {
  Q_ASSERT(from.within(sourceDimensions));
  return mappings[from.row * sourceDimensions.cols + from.col];
}

void NNF::setMapping(const ImageCoordinates &from, const ImageCoordinates &to) {
  Q_ASSERT(from.within(sourceDimensions) && to.within(targetDimensions));
  mappings[from.row * sourceDimensions.cols + from.col] = to;
}
