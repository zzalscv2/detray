/** Detray library, part of the ACTS project (R&D line)
 *
 * (c) 2020-2023 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

#include <gtest/gtest.h>

#include <limits>

// detray test
#include "tests/common/test_defs.hpp"

// detray core
#include "detray/definitions/indexing.hpp"
#include "detray/grids/populator.hpp"

using namespace detray;

TEST(grids, replace_populator) {
    replace_populator<> replacer;
    dindex stored = 3u;
    replacer(stored, 2u);
    EXPECT_EQ(stored, 2u);

    replacer(stored, 42u);
    EXPECT_EQ(stored, 42u);
}

TEST(grids, complete_populator) {

    using cpopulator4 =
        complete_populator<dvector, djagged_vector, darray, dindex, false, 4u>;
    cpopulator4 completer;

    cpopulator4::store_value stored = {completer.m_invalid, completer.m_invalid,
                                       completer.m_invalid,
                                       completer.m_invalid};

    cpopulator4::store_value test = stored;
    test[0] = 9u;
    completer(stored, 9u);
    EXPECT_EQ(stored, test);

    test[1] = 3u;
    completer(stored, 3u);
    EXPECT_EQ(stored, test);

    using sort_cpopulator4 =
        complete_populator<dvector, djagged_vector, darray, dindex, true, 4u>;
    sort_cpopulator4 sort_completer;

    test = {0u, 3u, 9u, 1000u};
    sort_completer(stored, 1000u);
    sort_completer(stored, 0u);
    EXPECT_EQ(stored, test);
}

TEST(grids, attach_populator) {
    // Attch populator without sorting
    attach_populator<> attacher;
    attach_populator<>::store_value stored = {3u};
    attacher(stored, 2u);
    attach_populator<>::store_value test = {3u, 2u};
    EXPECT_EQ(stored, test);

    attacher(stored, 42u);
    test = {3u, 2u, 42u};
    EXPECT_EQ(stored, test);

    // Attach populator with sorting
    attach_populator<dvector, djagged_vector, darray, dindex, true>
        sort_attacher;
    sort_attacher(stored, 11u);
    test = {2u, 3u, 11u, 42u};
    EXPECT_EQ(stored, test);
}
