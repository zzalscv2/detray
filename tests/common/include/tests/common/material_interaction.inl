/** Detray library, part of the ACTS project (R&D line)
 *
 * (c) 2022 CERN for the benefit of the ACTS project
 *
 * Mozilla Public License Version 2.0
 */

// Project include(s).
#include "detray/definitions/pdg_particle.hpp"
#include "detray/definitions/units.hpp"
#include "detray/detectors/create_telescope_detector.hpp"
#include "detray/materials/interaction.hpp"
#include "detray/materials/material.hpp"
#include "detray/materials/material_slab.hpp"
#include "detray/materials/predefined_materials.hpp"
#include "detray/propagator/actor_chain.hpp"
#include "detray/propagator/actors/aborters.hpp"
#include "detray/propagator/actors/parameter_resetter.hpp"
#include "detray/propagator/actors/parameter_transporter.hpp"
#include "detray/propagator/actors/pointwise_material_interactor.hpp"
#include "detray/propagator/navigator.hpp"
#include "detray/propagator/propagator.hpp"
#include "detray/propagator/rk_stepper.hpp"
#include "detray/simulation/random_scatterer.hpp"
#include "detray/utils/statistics.hpp"
#include "tests/common/tools/inspectors.hpp"

// VecMem include(s).
#include <vecmem/memory/host_memory_resource.hpp>

// GTest include(s).
#include <gtest/gtest.h>

using namespace detray;

using transform3 = __plugin::transform3<scalar>;
using matrix_operator = typename transform3::matrix_actor;

// Test class for MUON energy loss with Bethe function
// Input tuple: < material / energy / expected output from
// https://pdg.lbl.gov/2022/AtomicNuclearProperties for Muon dEdX and range >
class EnergyLossBetheValidation
    : public ::testing::TestWithParam<
          std::tuple<material<scalar>, scalar, scalar>> {};

// This tests the material functionalities
TEST_P(EnergyLossBetheValidation, bethe_energy_loss) {

    // Interaction object
    interaction<scalar> I;

    // intersection with a zero incidence angle
    line_plane_intersection is;

    // H2 liquid with a unit thickness
    material_slab<scalar> slab(std::get<0>(GetParam()), 1.f * unit<scalar>::cm);

    // muon
    constexpr int pdg = pdg_particle::eMuon;

    // mass
    constexpr scalar m{105.7f * unit<scalar>::MeV};

    // qOverP
    const scalar qOverP{-1.f / std::get<1>(GetParam())};

    // Bethe Stopping power in MeV * cm^2 / g
    const scalar dEdx{
        I.compute_energy_loss_bethe(is, slab, pdg, m, qOverP, -1.f) /
        slab.path_segment(is) / slab.get_material().mass_density() /
        (unit<scalar>::MeV * unit<scalar>::cm2 / unit<scalar>::g)};

    // Check if difference is within 5% error
    EXPECT_TRUE(std::abs(std::get<2>(GetParam()) - dEdx) / dEdx < 0.05f);
}

INSTANTIATE_TEST_SUITE_P(
    Bethe_0p1GeV_H2Liquid, EnergyLossBetheValidation,
    ::testing::Values(std::make_tuple(hydrogen_liquid<scalar>(),
                                      0.1003f * unit<scalar>::GeV, 6.539f)));

INSTANTIATE_TEST_SUITE_P(
    Bethe_1GeV_H2Liquid, EnergyLossBetheValidation,
    ::testing::Values(std::make_tuple(hydrogen_liquid<scalar>(),
                                      1.101f * unit<scalar>::GeV, 4.182f)));

INSTANTIATE_TEST_SUITE_P(
    Bethe_10GeV_H2Liquid, EnergyLossBetheValidation,
    ::testing::Values(std::make_tuple(hydrogen_liquid<scalar>(),
                                      10.11f * unit<scalar>::GeV, 4.777f)));

INSTANTIATE_TEST_SUITE_P(
    Bethe_100GeV_H2Liquid, EnergyLossBetheValidation,
    ::testing::Values(std::make_tuple(hydrogen_liquid<scalar>(),
                                      100.1f * unit<scalar>::GeV, 5.305f)));

INSTANTIATE_TEST_SUITE_P(
    Bethe_0p1GeV_HeGas, EnergyLossBetheValidation,
    ::testing::Values(std::make_tuple(helium_gas<scalar>(),
                                      0.1003f * unit<scalar>::GeV, 3.082f)));

INSTANTIATE_TEST_SUITE_P(
    Bethe_1GeV_HeGas, EnergyLossBetheValidation,
    ::testing::Values(std::make_tuple(helium_gas<scalar>(),
                                      1.101f * unit<scalar>::GeV, 2.133f)));

INSTANTIATE_TEST_SUITE_P(
    Bethe_10GeV_HeGas, EnergyLossBetheValidation,
    ::testing::Values(std::make_tuple(helium_gas<scalar>(),
                                      10.11f * unit<scalar>::GeV, 2.768f)));

INSTANTIATE_TEST_SUITE_P(
    Bethe_100GeV_HeGas, EnergyLossBetheValidation,
    ::testing::Values(std::make_tuple(helium_gas<scalar>(),
                                      100.1f * unit<scalar>::GeV, 3.188f)));

INSTANTIATE_TEST_SUITE_P(
    Bethe_0p1GeV_Al, EnergyLossBetheValidation,
    ::testing::Values(std::make_tuple(aluminium<scalar>(),
                                      0.1003f * unit<scalar>::GeV, 2.533f)));

INSTANTIATE_TEST_SUITE_P(
    Bethe_1GeV_Al, EnergyLossBetheValidation,
    ::testing::Values(std::make_tuple(aluminium<scalar>(),
                                      1.101f * unit<scalar>::GeV, 1.744f)));

INSTANTIATE_TEST_SUITE_P(
    Bethe_10GeV_Al, EnergyLossBetheValidation,
    ::testing::Values(std::make_tuple(aluminium<scalar>(),
                                      10.11f * unit<scalar>::GeV, 2.097f)));

INSTANTIATE_TEST_SUITE_P(
    Bethe_100GeV_Al, EnergyLossBetheValidation,
    ::testing::Values(std::make_tuple(aluminium<scalar>(),
                                      100.1f * unit<scalar>::GeV, 2.360f)));

INSTANTIATE_TEST_SUITE_P(
    Bethe_0p1GeV_Si, EnergyLossBetheValidation,
    ::testing::Values(std::make_tuple(silicon<scalar>(),
                                      0.1003f * unit<scalar>::GeV, 2.608f)));

INSTANTIATE_TEST_SUITE_P(
    Bethe_1GeV_Si, EnergyLossBetheValidation,
    ::testing::Values(std::make_tuple(silicon<scalar>(),
                                      1.101f * unit<scalar>::GeV, 1.803f)));

INSTANTIATE_TEST_SUITE_P(
    Bethe_10GeV_Si, EnergyLossBetheValidation,
    ::testing::Values(std::make_tuple(silicon<scalar>(),
                                      10.11f * unit<scalar>::GeV, 2.177f)));

INSTANTIATE_TEST_SUITE_P(
    Bethe_100GeV_Si, EnergyLossBetheValidation,
    ::testing::Values(std::make_tuple(silicon<scalar>(),
                                      100.1f * unit<scalar>::GeV, 2.451f)));

// Test class for MUON energy loss with Landau function
// Input tuple: < material / energy / expected energy loss  / expected fwhm  >
class EnergyLossLandauValidation
    : public ::testing::TestWithParam<
          std::tuple<material<scalar>, scalar, scalar, scalar>> {};

TEST_P(EnergyLossLandauValidation, landau_energy_loss) {

    // Interaction object
    interaction<scalar> I;

    // intersection with a zero incidence angle
    line_plane_intersection is;

    // H2 liquid with a unit thickness
    material_slab<scalar> slab(std::get<0>(GetParam()),
                               0.17f * unit<scalar>::cm);

    // muon
    constexpr int pdg{pdg_particle::eMuon};

    // mass
    constexpr scalar m{105.7f * unit<scalar>::MeV};

    // qOverP
    const scalar qOverP{-1.f / std::get<1>(GetParam())};

    // Landau Energy loss in MeV
    const scalar dE{
        I.compute_energy_loss_landau(is, slab, pdg, m, qOverP, -1.f) /
        unit<scalar>::MeV};

    // Check if difference is within 5% error
    EXPECT_TRUE(std::abs(std::get<2>(GetParam()) - dE) / dE < 0.05f);

    // Landau Energy loss Fluctuation
    const scalar fwhm{
        I.compute_energy_loss_landau_fwhm(is, slab, pdg, m, qOverP, -1.f) /
        unit<scalar>::MeV};

    // Check if difference is within 10% error
    EXPECT_TRUE(std::abs(std::get<3>(GetParam()) - fwhm) / fwhm < 0.1f);
}

// Expected output from Fig 33.7 in RPP2018
INSTANTIATE_TEST_SUITE_P(Landau_10GeV_Silicon, EnergyLossLandauValidation,
                         ::testing::Values(std::make_tuple(
                             silicon<scalar>(), 10.f * unit<scalar>::GeV,
                             0.525f, 0.13f)));

// Material interaction test with telescope Geometry
TEST(material_interaction, telescope_geometry_energy_loss) {

    vecmem::host_memory_resource host_mr;

    // Build from given module positions
    detail::ray<transform3> traj{{0.f, 0.f, 0.f}, 0.f, {1.f, 0.f, 0.f}, -1.f};
    std::vector<scalar> positions = {0.f,   50.f,  100.f, 150.f, 200.f, 250.f,
                                     300.f, 350.f, 400.f, 450.f, 500.f};

    const auto mat = silicon_tml<scalar>();
    constexpr scalar thickness{0.17f * unit<scalar>::cm};

    const auto det = create_telescope_detector(
        host_mr, positions, traj, 20.f * unit<scalar>::mm,
        20.f * unit<scalar>::mm, mat, thickness);

    using navigator_t = navigator<decltype(det)>;
    using constraints_t = constrained_step<>;
    using policy_t = stepper_default_policy;
    using stepper_t = line_stepper<transform3, constraints_t, policy_t>;
    using interactor_t = pointwise_material_interactor<transform3>;
    using actor_chain_t =
        actor_chain<dtuple, propagation::print_inspector, pathlimit_aborter,
                    parameter_transporter<transform3>, interactor_t,
                    parameter_resetter<transform3>>;
    using propagator_t = propagator<stepper_t, navigator_t, actor_chain_t>;

    // Propagator is built from the stepper and navigator
    propagator_t p({}, {});

    constexpr scalar q{-1.f};
    constexpr scalar iniP{10.f * unit<scalar>::GeV};

    typename bound_track_parameters<transform3>::vector_type bound_vector;
    getter::element(bound_vector, e_bound_loc0, 0) = 0.f;
    getter::element(bound_vector, e_bound_loc1, 0) = 0.f;
    getter::element(bound_vector, e_bound_phi, 0) = 0.f;
    getter::element(bound_vector, e_bound_theta, 0) = constant<scalar>::pi_2;
    getter::element(bound_vector, e_bound_qoverp, 0) = q / iniP;
    getter::element(bound_vector, e_bound_time, 0) = 0.f;
    typename bound_track_parameters<transform3>::covariance_type bound_cov =
        matrix_operator().template zero<e_bound_size, e_bound_size>();

    // bound track parameter at first physical plane
    const bound_track_parameters<transform3> bound_param(1u, bound_vector,
                                                         bound_cov);

    propagation::print_inspector::state print_insp_state{};
    pathlimit_aborter::state aborter_state{};
    parameter_transporter<transform3>::state bound_updater{};
    interactor_t::state interactor_state{};
    parameter_resetter<transform3>::state parameter_resetter_state{};

    // Create actor states tuples
    auto actor_states = std::tie(print_insp_state, aborter_state, bound_updater,
                                 interactor_state, parameter_resetter_state);

    propagator_t::state state(bound_param, det);

    // Propagate the entire detector
    ASSERT_TRUE(p.propagate(state, actor_states))
        << print_insp_state.to_string() << std::endl;

    // muon
    const int pdg{interactor_state.pdg};

    // mass
    const scalar mass{interactor_state.mass};

    // new momentum
    const scalar newP{state._stepping._bound_params.charge() /
                      state._stepping._bound_params.qop()};

    // new energy
    const scalar newE{std::hypot(newP, mass)};

    // Initial energy
    const scalar iniE{std::hypot(iniP, mass)};

    // New qop variance
    const scalar new_var_qop{
        matrix_operator().element(state._stepping._bound_params.covariance(),
                                  e_bound_qoverp, e_bound_qoverp)};

    // Interaction object
    interaction<scalar> I;

    // intersection with a zero incidence angle
    line_plane_intersection is;

    // Same material used for default telescope detector
    material_slab<scalar> slab(mat, thickness);

    // Expected Bethe Stopping power for telescope geometry is estimated
    // as (number of planes * energy loss per plane assuming 1 GeV muon).
    // It is not perfectly precise as the track loses its energy during
    // propagation. However, since the energy loss << the track momentum,
    // the assumption is not very bad

    // -2 is required because the first and last surface is a portal
    const scalar dE{
        I.compute_energy_loss_bethe(is, slab, pdg, mass, q / iniP, q) *
        static_cast<scalar>(positions.size() - 2u)};

    // Check if the new energy after propagation is enough close to the
    // expected value
    EXPECT_NEAR(newE, iniE - dE, 1e-5f);

    const scalar sigma_qop{I.compute_energy_loss_landau_sigma_QOverP(
        is, slab, pdg, mass, q / iniP, q)};

    const scalar dvar_qop{sigma_qop * sigma_qop *
                          static_cast<scalar>(positions.size() - 1u)};

    EXPECT_NEAR(new_var_qop, dvar_qop, 1e-10f);

    // @todo: Validate the backward direction case as well?
}

// Material interaction test with telescope Geometry
TEST(material_interaction, telescope_geometry_scattering_angle) {
    vecmem::host_memory_resource host_mr;

    // Build from given module positions
    detail::ray<transform3> traj{{0.f, 0.f, 0.f}, 0.f, {1.f, 0.f, 0.f}, -1.f};
    std::vector<scalar> positions = {0.f, 1000.f * unit<scalar>::cm,
                                     2000.f * unit<scalar>::cm};

    const auto mat = silicon_tml<scalar>();
    constexpr scalar thickness{500.f * unit<scalar>::cm};
    // Use unbounded surfaces
    constexpr bool unbounded = true;

    const auto det = create_telescope_detector<unbounded>(
        host_mr, positions, traj, 2000.f * unit<scalar>::mm,
        2000.f * unit<scalar>::mm, mat, thickness);

    using navigator_t = navigator<decltype(det)>;
    using constraints_t = constrained_step<>;
    using policy_t = stepper_default_policy;
    using stepper_t = line_stepper<transform3, constraints_t, policy_t>;
    using interactor_t = pointwise_material_interactor<transform3>;
    using simulator_t = random_scatterer<interactor_t>;
    using material_actor_t = composite_actor<dtuple, interactor_t, simulator_t>;
    using actor_chain_t =
        actor_chain<dtuple, propagation::print_inspector, pathlimit_aborter,
                    parameter_transporter<transform3>, material_actor_t,
                    parameter_resetter<transform3>>;
    using propagator_t = propagator<stepper_t, navigator_t, actor_chain_t>;

    // Propagator is built from the stepper and navigator
    propagator_t p({}, {});

    constexpr scalar q{-1.f};
    constexpr scalar iniP{10.f * unit<scalar>::GeV};

    typename bound_track_parameters<transform3>::vector_type bound_vector;
    getter::element(bound_vector, e_bound_loc0, 0) = 0.f;
    getter::element(bound_vector, e_bound_loc1, 0) = 0.f;
    getter::element(bound_vector, e_bound_phi, 0) = 0.f;
    getter::element(bound_vector, e_bound_theta, 0) = constant<scalar>::pi_2;
    getter::element(bound_vector, e_bound_qoverp, 0) = q / iniP;
    getter::element(bound_vector, e_bound_time, 0) = 0.f;
    typename bound_track_parameters<transform3>::covariance_type bound_cov =
        matrix_operator().template zero<e_bound_size, e_bound_size>();

    // bound track parameter
    const bound_track_parameters<transform3> bound_param(1, bound_vector,
                                                         bound_cov);

    std::size_t n_samples{100000u};
    std::vector<scalar> phi_vec;
    std::vector<scalar> theta_vec;

    scalar ref_phi_var{0.f};
    scalar ref_theta_var{0.f};

    for (std::size_t i = 0u; i < n_samples; i++) {

        propagation::print_inspector::state print_insp_state{};
        pathlimit_aborter::state aborter_state{};
        parameter_transporter<transform3>::state bound_updater{};
        interactor_t::state interactor_state{};
        interactor_state.do_energy_loss = false;
        // Seed = sample id
        simulator_t::state simulator_state{i};
        parameter_resetter<transform3>::state parameter_resetter_state{};

        // Create actor states tuples
        auto actor_states = std::tie(print_insp_state, aborter_state,
                                     bound_updater, interactor_state,
                                     simulator_state, parameter_resetter_state);

        propagator_t::state state(bound_param, det);

        state._stepping().set_overstep_tolerance(-1000.f * unit<scalar>::um);

        // Propagate the entire detector
        ASSERT_TRUE(p.propagate(state, actor_states))
            << print_insp_state.to_string() << std::endl;

        const auto& final_params = state._stepping._bound_params;

        if (i == 0u) {
            const auto& covariance = final_params.covariance();
            ref_phi_var =
                matrix_operator().element(covariance, e_bound_phi, e_bound_phi);
            ref_theta_var = matrix_operator().element(covariance, e_bound_theta,
                                                      e_bound_theta);
        }

        phi_vec.push_back(final_params.phi());
        theta_vec.push_back(final_params.theta());
    }

    scalar phi_var{statistics::variance(phi_vec)};
    scalar theta_var{statistics::variance(theta_vec)};

    EXPECT_NEAR((phi_var - ref_phi_var) / ref_phi_var, 0.f, 0.05f);
    EXPECT_NEAR((theta_var - ref_theta_var) / ref_theta_var, 0.f, 0.05f);

    // To make sure that the varainces are not zero
    EXPECT_TRUE(ref_phi_var > 1e-4f && ref_theta_var > 1e-4f);
}
