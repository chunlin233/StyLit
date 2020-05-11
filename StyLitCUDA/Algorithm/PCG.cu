#include "PCG.cuh"

namespace StyLitCUDA {

namespace PCG {

__device__ void pcgAdvance(PCGState *rng) {
  rng->state = rng->state * 6364136223846793005ULL + rng->increment;
}

__device__ uint32_t pcgOutput(uint64_t state) {
  return (uint32_t)(((state >> 22u) ^ state) >> ((state >> 61u) + 22u));
}

__device__ uint32_t pcgRand(PCGState *rng) {
  uint64_t oldstate = rng->state;
  pcgAdvance(rng);
  return pcgOutput(oldstate);
}

__device__ void pcgInit(PCGState *rng, uint64_t seed, uint64_t stream) {
  rng->state = 0U;
  rng->increment = (stream << 1u) | 1u;
  pcgAdvance(rng);
  rng->state += seed;
  pcgAdvance(rng);
}

} /* namespace PCG */

} /* namespace StyLitCUDA */
