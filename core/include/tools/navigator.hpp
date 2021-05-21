
/** Detray library, part of the ACTS project (R&D line)
 * 
 * (c) 2021 CERN for the benefit of the ACTS project
 * 
 * Mozilla Public License Version 2.0
 */

#pragma once

#include "core/intersection.hpp"
#include "core/track.hpp"
#include "utils/indexing.hpp"
#include "utils/enumerate.hpp"
#include "tools/intersection_kernel.hpp"

namespace detray
{

    /** A void inpector that does nothing.
     * 
     * Inspectors can be plugged in to understand the
     * the current navigation state information.
     * 
     */
    struct void_inspector
    {
        template <typename state_type>
        void operator()(const state_type & /*ignored*/) {}
    };

    /** The navigator struct. 
     * 
     * It follows the Acts::Navigator sturcture of sequence of
     * - status()
     * - target()
     * calls.
     * 
     * @tparam detector_tupe is the type of the detector
     * @tparam inspector_type is a validation inspector 
     */
    template <typename detector_type, typename inspector_type = void_inspector>
    struct navigator
    {

        using surface = typename detector_type::surface;
        using surface_link = typename detector_type::surface_link;

        using portal = typename detector_type::portal;
        using portal_links = typename detector_type::portal_links;

        using context = typename detector_type::context;

        /** Navigation status flag */
        enum navigation_status : int
        {
            e_on_target = -3,
            e_abort = -2,
            e_unknown = -1,
            e_towards_surface = 0,
            e_on_surface = 1,
            e_towards_portal = 2,
            e_on_portal = 3,
        };

        /** Navigation trust level */
        enum navigation_trust_level : int
        {
            e_no_trust = 0,   // re-evalute the candidates all over
            e_fair_trust = 1, // re-evaluate the distance & order of the (preselected) candidates
            e_high_trust = 3, // re-evaluate the distance to the next candidate
            e_full_trust = 4  // trust fully: distance & next candidate
        };

        /** A nested navigation kernel struct which can be used for surfaces, portals,
         *  volumes a like.
         *  
         * @tparam object_type the type of the relevant object
         * @tparam candidate_type the type of the candidates in the list
         * @tparam links_type the type of the links the candidate is holding
         * 
         **/
        template <typename object_type, typename candidate_type, typename links_type>
        struct navigation_kernel
        {
            const object_type *on = nullptr;
            dvector<candidate_type> candidates = {};
            typename dvector<candidate_type>::iterator next = candidates.end();
            links_type links;

            /** Indicate that the kernel is empty */
            bool empty() const { return candidates.empty(); }

            /** Clear the kernel */
            void clear()
            {
                candidates.clear();
                next = candidates.end();
                links = links_type{};
                on = nullptr;
            }
        };

        /** A navigation state object used to cache the information of the
         * current navigation stream.
         **/
        struct navigation_state
        {
            /// Kernel for the surfaces
            navigation_kernel<surface, intersection, surface_link> surface_kernel;
            /// Kernel the portals
            navigation_kernel<portal, intersection, portal_links> portal_kernel;

            // Volume navigation
            dindex volume_index = dindex_invalid;

            /// Distance to next
            scalar distance_to_next = std::numeric_limits<scalar>::infinity();
            /// The on surface tolerance
            scalar on_surface_tolerance = 1e-5;

            /// The inspector type of this navigation engine
            inspector_type inspector;

            /// The navigation status
            navigation_status status = e_unknown;
            /// If a surface / portal is reached
            dindex current_index = dindex_invalid;

            /// The navigation trust level
            navigation_trust_level trust_level = e_no_trust;
        };

        __plugin::cartesian2 cart2;
        __plugin::polar2 pol2;
        __plugin::cylindrical2 cyl2;

        /// The detector in which we are moving
        detector_type detector;

        /** Constructor with move constructor
         * 
         * @param d the detector for this navigator
         */
        navigator(detector_type &&d) : detector(std::move(d)) {}

        /** Navigation status() call which established the current navigation information.
         * 
         * @param navigation is the navigation cache object
         * @param track is the track infromation
         * @param trust is a boolean that indicates if you can trust already the last estimate
         * 
         * @return a navigation status to be further decoded 
         **/
        void status(navigation_state &navigation, const track<context> &track)
        {
            // Retrieve the volume & set index
            // Retrieve the volume, either from valid index or through global search
            const auto &volume = (navigation.volume_index != dindex_invalid) ? detector.indexed_volume(navigation.volume_index)
                                                                             : detector.indexed_volume(track.pos);
            navigation.volume_index = volume.index();
            // Retrieve the kernels
            auto &surface_kernel = navigation.surface_kernel;
            auto &portal_kernel = navigation.portal_kernel;

            // The navigation is not yet initialized, or you lost trust in it
            if (navigation.volume_index == dindex_invalid or navigation.trust_level == e_no_trust)
            {
                // First try to get the surface candidates
                initialize_kernel(navigation, surface_kernel, track, volume.surfaces());
                // If no surfaces are to processed, initialize the portals
                if (surface_kernel.empty())
                {
                    initialize_kernel(navigation, portal_kernel, track, volume.portals());
                    check_volume_switch(navigation);
                }
                // Before returning, run through the inspector
                navigation.inspector(navigation);
                return;
            }

            // Update the surface kernel
            if (not is_exhausted(surface_kernel) and update_kernel(navigation, surface_kernel, track, volume.surfaces()))
            {
                navigation.inspector(navigation);
                return;
            }

            // Update the portal kernel
            update_kernel(navigation, portal_kernel, track, volume.portals());
            check_volume_switch(navigation);
            navigation.inspector(navigation);
            return;
        }

        /** Target function of the navigator, finds the next candidates 
         *   and set the distance to next
         * 
         * @param navigation is the navigation cache
         * @param track is the current track information
         * 
         * @return a navigaiton status
         **/
        void target(navigation_state &navigation, const track<context> &track)
        {
            // Full trust level from target() call
            if (navigation.trust_level == e_full_trust)
            {
                return;
            }

            // Retrieve the volume, either from valid index or through global search
            const auto &volume = (navigation.volume_index != dindex_invalid) ? detector.indexed_volume(navigation.volume_index)
                                                                             : detector.indexed_volume(track.pos);
            navigation.volume_index = volume.index();
            // Retrieve the kernels
            auto &surface_kernel = navigation.surface_kernel;
            auto &portal_kernel = navigation.portal_kernel;

            // High targetting level
            if (navigation.trust_level == e_high_trust)
            {
                // Surfaces are/were present
                if (not surface_kernel.empty())
                {
                    if (is_exhausted(surface_kernel))
                    {
                        // Clear the surface kernel
                        surface_kernel.clear();
                        navigation.trust_level = e_no_trust;
                        update_kernel(navigation, portal_kernel, track, volume.portals());
                        navigation.inspector(navigation);
                        return;
                    }
                    else if (update_kernel(navigation, surface_kernel, track, volume.surfaces()))
                    {
                        navigation.inspector(navigation);
                        return;
                    }
                }
                // Portals are present
                update_kernel(navigation, portal_kernel, track, volume.portals());
            }
            navigation.inspector(navigation);
        }

        /** Helper method to intersect all objects of a constituents store
         * 
         * @tparam kernel_t the type of the kernel: surfaces/portals
         * @tparam constituents the type of the associated constituents
         * 
         * @param navigation is the navigation cache object
         * @param kernel [in,out] the kernel to be checked/initialized 
         * @param track the track information 
         * @param constituens the constituents to be checked
         * 
         */
        template <typename kernel_t, typename constituents_t>
        void initialize_kernel(navigation_state &navigation,
                               kernel_t &kernel,
                               const track<context> &track,
                               const constituents_t &constituents)
        {

            // Get the type of the kernel via a const expression at compile time
            constexpr bool kSurfaceType = (std::is_same_v<kernel_t, navigation_kernel<surface, intersection, surface_link>>);

            // Get the number of candidates & run them throuth the kernel
            size_t n_objects = constituents.objects().size();
            // Return if you have no objects
            if (n_objects == 0)
            {
                return;
            }
            kernel.candidates.reserve(n_objects);
            const auto &transforms = constituents.transforms();
            const auto &masks = constituents.masks();
            // Loop over all indexed surfaces, intersect and fill
            for (auto si : sequence({0, n_objects - 1}))
            {
                const auto &object = constituents.indexed_object(si);
                auto [sfi, link] = intersect(track, object, transforms, masks);
                sfi.index = si;
                sfi.link = link[0];
                if (sfi.status == e_inside)
                {
                    navigation.status = kSurfaceType ? e_towards_surface : e_towards_portal;
                    kernel.candidates.push_back(sfi);
                }
            }
            sort_and_set(navigation, kernel);
        }

        /** Helper method to the update the next candidate intersection 
         * 
         * @tparam kernel_t the type of the kernel: surfaces/portals
         * @tparam constituents the type of the associated constituents
         * 
         * @param navigation [in, out] the navigation state
         * @param kernel [in,out] the kernel to be checked/initialized 
         * @param track the track information 
         * @param constituens the constituents to be checked
         * 
         * @return The break condition
         */
        template <typename kernel_t, typename constituents_t>
        bool update_kernel(navigation_state &navigation,
                           kernel_t &kernel,
                           const track<context> &track,
                           const constituents_t &constituents)
        {

            // If the kernel is empty - intitalize it
            if (kernel.empty())
            {
                initialize_kernel(navigation, kernel, track, constituents);
                return true;
            }

            // Get the type of the kernel via a const expression at compile time
            constexpr bool kSurfaceType = (std::is_same_v<kernel_t, navigation_kernel<surface, intersection, surface_link>>);

            const auto &transforms = constituents.transforms();
            const auto &masks = constituents.masks();

            // Update current candidate, or step further
            // - do this only when you trust level is high
            if (navigation.trust_level >= e_high_trust and kernel.next != kernel.candidates.end())
            {
                // Only update the last intersection
                dindex si = kernel.next->index;
                const auto &s = constituents.indexed_object(si);
                auto [sfi, link] = intersect(track, s, transforms, masks);
                sfi.index = si;
                sfi.link = link[0];
                if (sfi.status == e_inside)
                {
                    // Update the intersection with a new one
                    (*kernel.next) = sfi;
                    navigation.distance_to_next = sfi.path;
                    if (sfi.path < navigation.on_surface_tolerance)
                    {
                        navigation.status = kSurfaceType ? e_on_surface : e_on_portal;
                        navigation.current_index = si;
                        if (navigation.status != e_on_portal)
                        {
                            ++kernel.next;
                            // Trust level is high
                            navigation.trust_level = e_high_trust;
                        }
                    }
                    else
                    {
                        navigation.status = kSurfaceType ? e_towards_surface : e_towards_portal;
                        // Trust fully again
                        navigation.trust_level = e_full_trust;
                    }
                    return true;
                }
                // If not successful: increase and switch to next
                ++kernel.next;
                if (update_kernel(navigation, kernel, track, constituents))
                {
                    return true;
                }
            }
            // Loop over all candidates and intersect again all candidates
            // - do this when your trust level is low
            else if (navigation.trust_level == e_fair_trust)
            {
                for (auto &c : kernel.candidates)
                {
                    dindex si = c.index;
                    const auto &s = constituents.indexed_object(si);
                    auto [sfi, link] = intersect(track, s, transforms, masks);
                    c = sfi;
                    c.index = si;
                    c.link = link[0];
                }
                sort_and_set(navigation, kernel);
                if (navigation.trust_level == e_high_trust)
                {
                    return true;
                }
            }
            // This kernel is exhausted
            kernel.next = kernel.candidates.end();
            navigation.trust_level = e_no_trust;
            return false;
        }

        /** Helper method to sort within the kernel 
         *
         * @param navigation is the state for setting the distance to next
         * @param kernel is the kernel which should be updated
         */
        template <typename kernel_t>
        void sort_and_set(navigation_state &navigation, kernel_t &kernel)
        {
            // Get the type of the kernel via a const expression at compile time
            constexpr bool kSurfaceType = (std::is_same_v<kernel_t, navigation_kernel<surface, intersection, surface_link>>);

            // Sort and set distance to next & navigation status
            if (not kernel.candidates.empty())
            {
                navigation.trust_level = e_full_trust;
                std::sort(kernel.candidates.begin(), kernel.candidates.end());
                kernel.next = kernel.candidates.begin();
                navigation.distance_to_next = kernel.next->path;
                if (navigation.distance_to_next < navigation.on_surface_tolerance)
                {
                    navigation.status = kSurfaceType ? e_on_surface : e_on_portal;
                    navigation.current_index = kernel.next->index;
                }
                navigation.current_index = dindex_invalid;
                navigation.status = kSurfaceType ? e_towards_surface : e_towards_portal;
            }
        }

        /** Helper method to check and perform a volume switch
         *
         * @param navigation is the navigation state 
         */
        void check_volume_switch(navigation_state &navigation)
        {
            // On a portal: switch volume index and (re-)initialize
            if (navigation.status == e_on_portal)
            {
                // Set volume index to the next volume provided by the portal
                navigation.volume_index = navigation.portal_kernel.next->link;
                navigation.surface_kernel.clear();
                navigation.portal_kernel.clear();
                navigation.trust_level = e_no_trust;
            }
        }

        /** Helper method to check if a kernel is exhaused 
         * 
         * @tparam kernel_t the type of the kernel
         * @param kernel the kernel to be checked
         * 
         * @return true if the kernel is exhaused 
         */
        template <typename kernel_t>
        bool is_exhausted(const kernel_t &kernel)
        {
            return (kernel.next == kernel.candidates.end());
        }
    };

} // namespace detray