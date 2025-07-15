#pragma once

#include <type_traits>

namespace twilight
{
template <typename E>
inline E operator|(E a, E b) noexcept
  requires std::is_enum_v<E>
{
  using T = std::underlying_type_t<E>;
  return static_cast<E>(static_cast<T>(a) | static_cast<T>(b));
}

template <typename E>
inline E operator&(E a, E b) noexcept
  requires std::is_enum_v<E>
{
  using T = std::underlying_type_t<E>;
  return static_cast<E>(static_cast<T>(a) & static_cast<T>(b));
}

template <typename E>
inline E operator~(E a) noexcept
  requires std::is_enum_v<E>
{
  using T = std::underlying_type_t<E>;
  return static_cast<E>(~static_cast<T>(a));
}

template <typename E>
inline E operator^(E a, E b) noexcept
  requires std::is_enum_v<E>
{
  using T = std::underlying_type_t<E>;
  return static_cast<E>(static_cast<T>(a) ^ static_cast<T>(b));
}

template <typename E>
inline E& operator|=(E& a, E b) noexcept
  requires std::is_enum_v<E>
{
  return a = a | b;
}

template <typename E>
inline E& operator&=(E& a, E b) noexcept
  requires std::is_enum_v<E>
{
  return a = a & b;
}

template <typename E>
inline E& operator^=(E& a, E b) noexcept
  requires std::is_enum_v<E>
{
  return a = a ^ b;
}

template <typename E>
inline bool operator!(E a) noexcept
  requires std::is_enum_v<E>
{
  using T = std::underlying_type_t<E>;
  return !static_cast<T>(a);
}
}  // namespace twilight
