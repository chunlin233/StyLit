#ifndef STYLITCOORDINATORCPU_H
#define STYLITCOORDINATORCPU_H

#include "Algorithm/Pyramid.h"
#include "Algorithm/PyramidLevel.h"
#include "Algorithm/StyLitCoordinator.h"
#include "DownscalerCPU.h"
#include "NNFApplicatorCPU.h"
#include "NNFGeneratorCPU.h"
#include "NNFUpscalerCPU.h"
#include "Utilities/ImageIO.h"

#include <QString>
#include <iostream>

template <unsigned int numGuideChannels, unsigned int numStyleChannels>
class StyLitCoordinatorCPU
    : public StyLitCoordinator<float, numGuideChannels, numStyleChannels> {
public:
  StyLitCoordinatorCPU() = default;
  ~StyLitCoordinatorCPU() = default;

  // If an image is shrunken to be smaller than this, StyLit fails.
  const int MINIMUM_IMAGE_AREA = 100;

  /**
   * @brief runStyLit The implementation-specific StyLit implementation is
   * called from here.
   * @return true if StyLit ran successfully; otherwise false
   */
  bool runStyLit(const Configuration &configuration) {
    startTimer();
    std::cout << std::endl << std::endl;
    printTime("Starting runStyLit in StyLitCoordinatorCPU.");
    Pyramid<float, numGuideChannels, numStyleChannels> pyramid;

    //srand(4);

    // Gets the highest pyramid level's dimensions.
    ImageDimensions sourceDimensions;
    if (!ImageIO::getImageDimensions(configuration.sourceGuideImagePaths[0],
                                     sourceDimensions)) {
      std::cerr << "Could not read input images." << std::endl;
      return false;
    }
    ImageDimensions targetDimensions;
    if (!ImageIO::getImageDimensions(configuration.targetGuideImagePaths[0],
                                     targetDimensions)) {
      std::cerr << "Could not read input images." << std::endl;
      return false;
    }

    // Creates and reads in the highest pyramid level.
    pyramid.levels.emplace_back(sourceDimensions, targetDimensions);
    if (!ImageIO::readPyramidLevel<numGuideChannels, numStyleChannels>(
            configuration, pyramid.levels[0])) {
      std::cerr << "Could not read input images." << std::endl;
      return false;
    }
    printTime("Done reading A, B and A'.");

    // Adds the guide and style weights.
    unsigned int guideChannel = 0;
    for (unsigned int i = 0; i < configuration.guideImageFormats.size(); i++) {
      const int numChannels =
          ImageFormatTools::numChannels(configuration.guideImageFormats[i]);
      for (int j = 0; j < numChannels; j++) {
        pyramid.guideWeights[guideChannel++] =
            configuration.guideImageWeights[i];
      }
    }
    Q_ASSERT(guideChannel == numGuideChannels);
    unsigned int styleChannel = 0;
    for (unsigned int i = 0; i < configuration.styleImageFormats.size(); i++) {
      const int numChannels =
          ImageFormatTools::numChannels(configuration.styleImageFormats[i]);
      for (int j = 0; j < numChannels; j++) {
        pyramid.styleWeights[styleChannel++] =
            configuration.styleImageWeights[i];
      }
    }
    Q_ASSERT(styleChannel == numStyleChannels);

    // Downscales the pyramid levels.
    DownscalerCPU<float, numGuideChannels> guideDownscaler;
    DownscalerCPU<float, numStyleChannels> styleDownscaler;
    int factor = 2;
    for (int level = 1; level < configuration.numPyramidLevels; level++) {
      // Calculates the pyramid level's image size and checks its validity.
      const ImageDimensions scaledSourceDimensions = sourceDimensions / factor;
      const ImageDimensions scaledTargetDimensions = targetDimensions / factor;
      if (scaledSourceDimensions.area() < MINIMUM_IMAGE_AREA) {
        fprintf(stderr,
                "At pyramid level %d, the source image area (%d) is invalid "
                "because it is below the minimum of %d. Consider reducing the "
                "number of pyramid levels.",
                level, scaledSourceDimensions.area(), MINIMUM_IMAGE_AREA);
        return false;
      }
      if (scaledTargetDimensions.area() < MINIMUM_IMAGE_AREA) {
        fprintf(stderr,
                "At pyramid level %d, the target image area (%d) is invalid "
                "because it is below the minimum of %d. Consider reducing the "
                "number of pyramid levels.",
                level, scaledTargetDimensions.area(), MINIMUM_IMAGE_AREA);
        return false;
      }

      // Allocates the next-lowest pyramid level.
      pyramid.levels.emplace_back(scaledSourceDimensions,
                                  scaledTargetDimensions);
      factor *= 2;

      // Downscales the images.
      guideDownscaler.downscale(configuration,
                                pyramid.levels[level - 1].guide.source,
                                pyramid.levels[level].guide.source);
      guideDownscaler.downscale(configuration,
                                pyramid.levels[level - 1].guide.target,
                                pyramid.levels[level].guide.target);
      styleDownscaler.downscale(configuration,
                                pyramid.levels[level - 1].style.source,
                                pyramid.levels[level].style.source);
    }
    printTime("Done downscaling A, B and A'.");

    // get an NNFApplicator
    NNFApplicatorCPU<float, numGuideChannels, numStyleChannels> nnfApplicator;

    // Sets B' in the lowest level to be initialized from A' and a randomly
    // initialized NNF
    PatchMatcherCPU<float, numGuideChannels, numStyleChannels> patchMatcher;
    patchMatcher.randomlyInitializeNNF(
        pyramid.levels[int(pyramid.levels.size()) - 1].forwardNNF);
    nnfApplicator.applyNNF(configuration,
                           pyramid.levels[int(pyramid.levels.size()) - 1]);

    // Generates NNFs from the coarsest to the finest level.
    NNFGeneratorCPU<float, numGuideChannels, numStyleChannels> generator;
    NNFUpscalerCPU nnfUpscaler;

    for (int level = int(pyramid.levels.size()) - 1; level >= 0; level--) {
      PyramidLevel<float, numGuideChannels, numStyleChannels> &pyramidLevel =
          pyramid.levels[level];

      if (level < int(pyramid.levels.size()) - 1) {
        // If not at the coarsest level, upscales the NNF and applies it to make
        // the next-finest B'.
        PyramidLevel<float, numGuideChannels, numStyleChannels>
            &previousPyramidLevel = pyramid.levels[level + 1];
        nnfUpscaler.upscaleNNF(configuration, previousPyramidLevel.forwardNNF,
                               pyramidLevel.forwardNNF);
        nnfApplicator.applyNNF(configuration, pyramidLevel);
      }

      std::vector<float> budgets;
      for (int i = 0; i < configuration.numOptimizationIterationsPerPyramidLevel; i++) {
        generator.generateNNF(configuration, pyramid, level, budgets);
        printTime("Done with generating NNF.");
        nnfApplicator.applyNNF(configuration, pyramidLevel);
        printTime("Done applying NNF.");
      }

      // Saves an image.
      QString location = configuration.targetStyleImagePaths[0];
      location += QString::number(level);
      location += ".png";
      ImageIO::writeImage<numStyleChannels>(location, pyramidLevel.style.target,
                                            ImageFormat::RGB, 0);

      printTime("Done with pyramid level.");
    }

    // Runs the actual algorithm.
    printTime("Created final image");
    ErrorCalculatorCPU<float, numGuideChannels, numStyleChannels> thing;
    return true;
  }

private:
  // This is used to print execution times to stdout.
  std::chrono::time_point<std::chrono::steady_clock> startTime;

  // Starts the timer.
  void startTimer() { startTime = std::chrono::steady_clock::now(); }

  /**
   * @brief printTime Prints the number of milliseconds since the start of
   * program execution along with the event.
   * @param event the event string
   */
  void printTime(const QString &event) {
    auto end = std::chrono::steady_clock::now();
    auto diff = end - startTime;
    std::cout << std::chrono::duration<double, std::milli>(diff).count()
              << " ms: " << event.toLocal8Bit().constData() << std::endl;
  }
};

#endif // STYLITCOORDINATORCPU_H
