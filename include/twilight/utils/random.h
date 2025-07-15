#pragma once

#include <array>
#include <random>

#include "types.h"

namespace twilight
{
template <typename T, usize N = 1>
inline std::conditional_t<N == 1, T, std::array<T, N>> rand() noexcept
{
  static_assert(std::is_integral_v<T>, "T must be an integral type");

  static std::mt19937 rng(std::random_device{}());
  static std::uniform_int_distribution<T> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());

  if constexpr (N == 1) {
    return dist(rng);
  } else {
    std::array<T, N> arr{};
    for (T& x : arr) x = dist(rng);
    return arr;
  }
}
}  // namespace twilight
