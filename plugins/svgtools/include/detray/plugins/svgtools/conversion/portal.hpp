/** Detray library, part of the ACTS project (R&D line)
 *
 * (c) 2023 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

#pragma once

// Project include(s)
#include "detray/geometry/surface.hpp"
#include "detray/plugins/svgtools/conversion/link.hpp"
#include "detray/plugins/svgtools/conversion/surface.hpp"
#include "detray/plugins/svgtools/styling/styling.hpp"
#include "detray/plugins/svgtools/utils/link_utils.hpp"

// Actsvg includes(s)
#include "actsvg/proto/portal.hpp"

namespace detray::svgtools::conversion {

/// @returns An actsvg proto portal representing the portal.
/// @note detray portal is_portal() should be true.
template <typename detector_t>
auto portal(const typename detector_t::geometry_context& context,
            const detector_t& detector,
            const detray::surface<detector_t>& d_portal,
            const styling::portal_style& style =
                styling::tableau_colorblind::portal_style,
            bool hide_links = false) {

    assert(d_portal.is_portal());

    using point3_container_t = std::vector<typename detector_t::point3_type>;
    using p_portal_t = actsvg::proto::portal<point3_container_t>;

    p_portal_t p_portal;
    p_portal._name = "portal_" + std::to_string(d_portal.index());
    if (!hide_links && svgtools::utils::is_not_world_portal(d_portal)) {
        p_portal._volume_links = {
            svgtools::conversion::link(context, detector, d_portal)};
    }

    p_portal._surface = svgtools::conversion::surface(context, d_portal);

    svgtools::styling::apply_style(p_portal, style);

    return p_portal;
}

}  // namespace detray::svgtools::conversion
