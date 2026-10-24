/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file Format.cppm
 * @brief Partition `aest.colors:format`: the std::formatter specialization
 *        for every ColorType.
 */
module;
#include <algorithm>
#include <format>
#include <initializer_list>
#include <string_view>
export module aest.colors:format;

import :color;

/**
 * @brief Formats a color as `TYPE(ch1, ch2, ch3, alpha)`.
 *
 * The format spec is a floating-point spec applied to every channel, so
 * `{:.1f}` prints one decimal each; an empty spec prints three decimals.
 * @tparam Color The color type to format
 */
template <aest::colors::ColorType Color>
struct std::formatter<Color>
{
    /**
     * @brief Parse the spec that every channel is formatted with.
     * @param[in,out] ctx The parse context, positioned at the spec
     * @return The iterator past the parsed spec.
     */
    constexpr auto parse(std::format_parse_context& ctx)
    {
        _useDefault = ctx.begin() == ctx.end() || *ctx.begin() == '}';
        return _useDefault ? ctx.begin() : _channel.parse(ctx);
    }

    /**
     * @brief Write the formatted color to the output of @p ctx.
     * @param[in] color The color to format
     * @param[in,out] ctx The format context to write to
     * @return The output iterator past the written characters.
     */
    auto format(const Color& color, std::format_context& ctx) const
    {
        auto out = std::format_to(ctx.out(), "{}(", Color::TYPE);
        std::string_view separator;
        for (const float value : {color.ch1(), color.ch2(), color.ch3(), color.alpha()})
        {
            out       = std::ranges::copy(separator, out).out;
            separator = ", ";
            if (_useDefault)
            {
                out = std::format_to(out, "{:.3f}", value);
            }
            else
            {
                ctx.advance_to(out);
                out = _channel.format(value, ctx);
            }
        }
        *out++ = ')';
        return out;
    }

private:
    std::formatter<float> _channel; ///< Formatter for each channel, set by a non-empty spec
    bool _useDefault = true;        ///< Whether the spec was empty, printing three decimals
};
