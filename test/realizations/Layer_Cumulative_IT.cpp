// This integration test constructs ngen::Layer directly against
// hy_features::HY_Features. Under MPI builds, ngen::Layer::feature_type is
// HY_Features_MPI (an unrelated class requiring PartitionData/mpi_rank wiring),
// so this fixture cannot compile against the MPI layer. The Layer scaling math
// being exercised here is identical between serial and MPI builds, so serial
// coverage is sufficient. Revisit if MPI-only logic is added to Layer.
#include <NGenConfig.h>
#if !NGEN_WITH_MPI

#include "gtest/gtest.h"

#include <Layer.hpp>
#include <LayerData.hpp>
#include <Simulation_Time.hpp>
#include <Formulation_Manager.hpp>
#include <Catchment_Formulation.hpp>
#include <HY_Features.hpp>
#include <HY_PointHydroNexus.hpp>

#include <FeatureCollection.hpp>
#include <features/Features.hpp>
#include <JSONGeometry.hpp>
#include <JSONProperty.hpp>

#include <boost/property_tree/ptree.hpp>

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

/**
 * Catchment_Formulation stub whose per-timestep get_response output is taken
 * from a caller-supplied vector indexed by the time step index. Used by the
 * cumulative scaling regression test to drive Layer::update_models across
 * multiple timesteps with values that differ per step, so a scaling error at
 * any one step is observable rather than masked by uniform input.
 */
class VaryingStubCatchmentFormulation : public realization::Catchment_Formulation
{
public:
    VaryingStubCatchmentFormulation(std::string id, std::vector<double> per_step)
        : realization::Catchment_Formulation(std::move(id)),
          responses(std::move(per_step))
    {
        // Output stream left default-constructed; it begins as a null stream
        // that discards writes. See test/core/Layer_Test.cpp for the
        // rationale on not calling set_output_stream("").
    }

    std::string get_formulation_type() const override { return "stub"; }

    double get_response(time_step_t t_index, time_step_t /*t_delta*/) override
    {
        return responses.at(static_cast<size_t>(t_index));
    }

    std::string get_output_line_for_timestep(int /*timestep*/,
        std::string /*delimiter*/ = DEFAULT_FORMULATION_OUTPUT_DELIMITER) override
    {
        return "";
    }

    const std::vector<std::string>& get_required_parameters() const override
    {
        static const std::vector<std::string> empty;
        return empty;
    }

    void create_formulation(boost::property_tree::ptree& /*config*/,
                            geojson::PropertyMap* /*global*/ = nullptr) override {}

    void create_formulation(geojson::PropertyMap /*properties*/) override {}

    void check_mass_balance(const int& /*iteration*/,
                            const int& /*total_steps*/,
                            const std::string& /*timestamp*/) const override {}

private:
    std::vector<double> responses;
};

geojson::Feature make_catchment_feature(const std::string& id,
                                        const std::string& downstream_nexus_id,
                                        double area_sqkm)
{
    geojson::three_dimensional_coordinates coords{
        {
            {0.0, 0.0},
            {1.0, 0.0},
            {1.0, 1.0},
            {0.0, 1.0},
            {0.0, 0.0}
        }
    };
    geojson::PropertyMap properties{
        {"toid",     geojson::JSONProperty("toid", downstream_nexus_id)},
        {"areasqkm", geojson::JSONProperty("areasqkm", area_sqkm)}
    };
    return std::make_shared<geojson::PolygonFeature>(
        geojson::PolygonFeature(geojson::polygon(coords), id, properties));
}

geojson::Feature make_nexus_feature(const std::string& id)
{
    geojson::PropertyMap properties{};
    return std::make_shared<geojson::PointFeature>(
        geojson::PointFeature(geojson::coordinate_t(0.0, 0.0), id, properties));
}

struct CumulativeRunResult {
    double cumulative_volume_m3 = 0.0;
    std::vector<double> per_step_flow_m3_per_s;
};

/**
 * Drive a one-catchment / one-nexus simulation across the full
 * Simulation_Time output schedule. Returns the cumulative volume in m^3 the
 * nexus accumulated (sum over steps of flow_m3_per_s * step_seconds) and the
 * per-step flow series for finer-grained diagnostics.
 */
CumulativeRunResult run_cumulative_simulation(
    const std::string& start_ts,
    const std::string& end_ts,
    int output_interval_seconds,
    const std::vector<double>& per_step_responses,
    double area_sqkm)
{
    const std::string catchment_id = "cat-1";
    const std::string nexus_id = "nex-1";

    auto fabric = std::make_shared<geojson::FeatureCollection>();
    fabric->add_feature(make_catchment_feature(catchment_id, nexus_id, area_sqkm));
    fabric->add_feature(make_nexus_feature(nexus_id));
    std::string link_key = "toid";
    fabric->link_features_from_property(nullptr, &link_key);

    std::stringstream cfg;
    cfg << R"({"disable_catchment_output": true})";
    auto formulations = std::make_shared<realization::Formulation_Manager>(cfg);
    formulations->add_formulation(
        std::make_shared<VaryingStubCatchmentFormulation>(catchment_id, per_step_responses));

    auto features = std::make_shared<hy_features::HY_Features>(fabric, formulations);

    simulation_time_params st_params(start_ts, end_ts, output_interval_seconds);
    Simulation_Time sim_time(st_params);

    ngen::LayerDescription desc{
        "test layer", "s", 0, static_cast<double>(output_interval_seconds)};
    std::vector<std::string> processing_units{catchment_id};
    ngen::Layer layer(desc, processing_units, sim_time, *features, fabric, 0);

    std::vector<double> empty_doubles;
    std::unordered_map<std::string, int> empty_map;

    const int total_steps = sim_time.get_total_output_times();
    for (int step_idx = 0; step_idx < total_steps; ++step_idx) {
        layer.update_models(
            boost::span<double>(empty_doubles), empty_map,
            boost::span<double>(empty_doubles), empty_map,
            step_idx);
    }

    auto nexus = features->nexus_at(nexus_id);
    CumulativeRunResult result;
    result.per_step_flow_m3_per_s.reserve(static_cast<size_t>(total_steps));
    for (int t = 0; t < total_steps; ++t) {
        const double flow_m3_per_s = nexus->inspect_upstream_flows(t).first;
        result.per_step_flow_m3_per_s.push_back(flow_m3_per_s);
        result.cumulative_volume_m3 += flow_m3_per_s * static_cast<double>(output_interval_seconds);
    }
    return result;
}

double sum_of(const std::vector<double>& xs)
{
    double s = 0.0;
    for (double x : xs) { s += x; }
    return s;
}

constexpr double kFlowTolerance = 1.0e-9;
constexpr double kVolumeRelTolerance = 1.0e-12;

} // namespace

// Scenario A: drive a multi-step simulation at the regression-target output
// interval of 900 seconds. With per-step responses that vary step-by-step,
// the cumulative volume at the nexus must equal area_m2 * sum(responses):
// the per-step (response * area_m2 / step) * step contributions telescope to
// area_m2 * response per step, independent of step length. Pre-fix code
// (divisor hard-coded to 3600) would inflate each per-step flow by 4x at
// 900s, breaking both the cumulative volume and per-step flow assertions.
TEST(LayerCumulativeScalingIT, CumulativeVolumeAcrossSteps_A_NinetySecond)
{
    constexpr double area_sqkm = 2.5;
    constexpr int step_seconds = 900;
    // 2-hour run at 900s yields total_output_times = 7200/900 + 1 = 9.
    const std::vector<double> responses{
        0.0010, 0.0020, 0.0015, 0.0030,
        0.0025, 0.0005, 0.0040, 0.0018, 0.0007};

    const auto result = run_cumulative_simulation(
        "2015-12-01 00:00:00", "2015-12-01 02:00:00",
        step_seconds, responses, area_sqkm);

    ASSERT_EQ(result.per_step_flow_m3_per_s.size(), responses.size());

    const double expected_volume_m3 = area_sqkm * 1.0e6 * sum_of(responses);
    EXPECT_NEAR(result.cumulative_volume_m3, expected_volume_m3,
                expected_volume_m3 * kVolumeRelTolerance);

    // Also assert the per-step flow series, which catches the failure mode
    // where over- and under-scaled steps could happen to sum to the right
    // total but be individually wrong.
    for (size_t i = 0; i < responses.size(); ++i) {
        const double expected_flow =
            responses[i] * area_sqkm * 1.0e6 / static_cast<double>(step_seconds);
        EXPECT_NEAR(result.per_step_flow_m3_per_s[i], expected_flow, kFlowTolerance)
            << " at step " << i;
    }
}

// Scenario B: hourly baseline. Same volume-conservation property; catches a
// future regression that re-introduces a divisor only numerically equivalent
// to the step length at 3600s.
TEST(LayerCumulativeScalingIT, CumulativeVolumeAcrossSteps_B_Hourly)
{
    constexpr double area_sqkm = 2.5;
    constexpr int step_seconds = 3600;
    // 2-hour run at 3600s yields total_output_times = 7200/3600 + 1 = 3.
    const std::vector<double> responses{0.0010, 0.0030, 0.0020};

    const auto result = run_cumulative_simulation(
        "2015-12-01 00:00:00", "2015-12-01 02:00:00",
        step_seconds, responses, area_sqkm);

    ASSERT_EQ(result.per_step_flow_m3_per_s.size(), responses.size());

    const double expected_volume_m3 = area_sqkm * 1.0e6 * sum_of(responses);
    EXPECT_NEAR(result.cumulative_volume_m3, expected_volume_m3,
                expected_volume_m3 * kVolumeRelTolerance);

    for (size_t i = 0; i < responses.size(); ++i) {
        const double expected_flow =
            responses[i] * area_sqkm * 1.0e6 / static_cast<double>(step_seconds);
        EXPECT_NEAR(result.per_step_flow_m3_per_s[i], expected_flow, kFlowTolerance)
            << " at step " << i;
    }
}

#endif // !NGEN_WITH_MPI
