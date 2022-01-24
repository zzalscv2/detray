/** Detray library, part of the ACTS project (R&D line)
 *
 * (c) 2021-2022 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */
#pragma once

#include <thrust/tuple.h>

#include <array>
#include <tuple>
#include <type_traits>
#include <utility>

#include "detray/definitions/qualifiers.hpp"

namespace detray {

/** define tuple type namespace for host (std) and device (thrust) compiler
 * **/
#if defined(__CUDACC__)
namespace vtuple = thrust;
#else
namespace vtuple = std;
#endif

namespace detail {

/** get function accessor
 *
 *  usage example:
 *  detail::get<0>(tuple)
 */
using std::get;
using thrust::get;

template <std::size_t id, typename mask_store_t,
          std::enable_if_t<std::is_class_v<typename std::remove_reference_t<
                               mask_store_t>::mask_tuple>,
                           bool> = true>
constexpr auto get(mask_store_t&& mask_store) noexcept
    -> decltype(get<id>(std::forward<mask_store_t>(mask_store).masks())) {
    return get<id>(std::forward<mask_store_t>(mask_store).masks());
}

/** tuple size accessor
 *
 *  usage example:
 *  detail::tuple_size< tuple_type >::value
 */
template <class T, typename Enable = void>
struct tuple_size;

// std::tuple
template <template <typename...> class tuple_type, class... value_types>
struct tuple_size<tuple_type<value_types...>,
                  typename std::enable_if_t<
                      std::is_same_v<tuple_type<value_types...>,
                                     std::tuple<value_types...>> == true>>
    : std::tuple_size<tuple_type<value_types...>> {};

// thrust::tuple
template <template <typename...> class tuple_type, class... value_types>
struct tuple_size<tuple_type<value_types...>,
                  typename std::enable_if_t<
                      std::is_same_v<tuple_type<value_types...>,
                                     thrust::tuple<value_types...>> == true>>
    : thrust::tuple_size<tuple_type<value_types...>> {};

/** make tuple accessor
 *  users have to specifiy tuple_type for detail::make_tuple
 *
 *  usage example
 *  detail::make_tuple<tuple_type>(args...)
 */

template <class T>
struct unwrap_refwrapper {
    using type = T;
};

template <class T>
struct unwrap_refwrapper<std::reference_wrapper<T>> {
    using type = T&;
};

template <class T>
using unwrap_decay_t =
    typename unwrap_refwrapper<typename std::decay<T>::type>::type;

template <template <typename...> class tuple_type, class... Types>
DETRAY_HOST_DEVICE constexpr tuple_type<unwrap_decay_t<Types>...> make_tuple(
    Types&&... args) {
    return tuple_type<unwrap_decay_t<Types>...>(std::forward<Types>(args)...);
}

}  // namespace detail
}  // namespace detray