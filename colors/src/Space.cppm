/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file Space.cppm
 * @brief Partition `aest.colors:space`: the tag that selects the color space
 *        an operation works in.
 */
export module aest.colors:space;

import :color;

export namespace aest::colors
{
/**
 * @brief Tag naming the color space an operation works in.
 * @tparam Space The color space.
 */
template <ColorType Space>
struct In
{};

/**
 * @brief Selects the color space an operation works in, e.g.
 * `lighten(color, 0.2f, in<HSL>)`.
 * @tparam Space The color space.
 */
template <ColorType Space>
inline constexpr In<Space> in{};
} // namespace aest::colors
