/** Detray library, part of the ACTS project (R&D line)
 *
 * (c) 2022 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

#pragma once

// Project include(s)
#include "detray/utils/ranges.hpp"

// System include(s).
#include <algorithm>
#include <numeric>
#include <type_traits>

namespace detray::statistics {

/// @returns the sample mean over a range of numbers
template <typename range_t,
          std::enable_if_t<detray::ranges::range_v<range_t>, bool> = true>
inline auto mean(const range_t& r) noexcept(false) {
    using value_t = detray::ranges::range_value_t<range_t>;

    value_t sum = std::accumulate(r.begin(), r.end(), value_t{});
    value_t mean = sum * (1.0f / r.size());

    return mean;
}

/// @returns the sample variance over a range of numbers
template <typename range_t,
          std::enable_if_t<detray::ranges::range_v<range_t>, bool> = true>
inline auto variance(const range_t& r) noexcept(false) {
    using value_t = detray::ranges::range_value_t<range_t>;

    value_t mean = statistics::mean(r);
    std::vector<value_t> diff(r.size());
    std::transform(r.begin(), r.end(), diff.begin(),
                   [mean](value_t x) { return x - mean; });
    value_t sq_sum =
        std::inner_product(diff.begin(), diff.end(), diff.begin(), value_t{});
    value_t variance = sq_sum * (1.0f / r.size());

    return variance;
}

}  // namespace detray::statistics