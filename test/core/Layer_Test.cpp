// These tests construct ngen::Layer directly against hy_features::HY_Features.
// Under MPI builds, ngen::Layer::feature_type is HY_Features_MPI (an unrelated
// class requiring PartitionData/mpi_rank wiring), so this fixture cannot
// compile against the MPI layer. The Layer::update_models scaling math being
// tested is identical between serial and MPI builds, so serial coverage is
// sufficient. Revisit if MPI-only logic is added to Layer::update_models.
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
 * Minimal Catchment_Formulation stub used to drive Layer::update_models with a
 * deterministic per-timestep response. Only the pure-virtual surface of the
 * Catchment_Formulation hierarchy is implemented; other behavior is left as a
 * no-op so the test exercises Layer's scaling math in isolation.
 */
class StubCatchmentFormulation : public realization::Catchment_Formulation
{
public:
    StubCatchmentFormulation(std::string id, double response)
        : realization::Catchment_Formulation(std::move(id)), fixed_response(response)
    {
        // Output stream is default-constructed to a null stream by
        // HY_CatchmentArea, which discards any writes Layer emits.
    }

    std::string get_formulation_type() const override { return "stub"; }

    double get_response(time_step_t /*t_index*/, time_step_t /*t_delta*/) override
    {
        return fixed_response;
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
    double fixed_response;
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

/**
 * Build a one-catchment / one-nexus simulation, drive Layer::update_models for
 * one step, and return the volumetric flow (m^3/s) the nexus recorded.
 *
 * @param output_interval_seconds The Simulation_Time output interval, also the
 *        layer's per-step duration.
 * @param formulation_response The depth (m/timestep) returned by the stub
 *        formulation from get_response.
 * @param area_sqkm Catchment area (square kilometers) attached to the feature.
 */
double run_layer_step_and_get_nexus_flow(int output_interval_seconds,
                                         double formulation_response,
                                         double area_sqkm)
{
    const std::string catchment_id = "cat-1";
    const std::string nexus_id = "nex-1";

    auto fabric = std::make_shared<geojson::FeatureCollection>();
    fabric->add_feature(make_catchment_feature(catchment_id, nexus_id, area_sqkm));
    fabric->add_feature(make_nexus_feature(nexus_id));
    std::string link_key = "toid";
    fabric->link_features_from_property(nullptr, &link_key);

    // Disable per-catchment csv output so HY_Features does not try to create
    // a real file for the stub formulation.
    std::stringstream cfg;
    cfg << R"({"disable_catchment_output": true})";
    auto formulations = std::make_shared<realization::Formulation_Manager>(cfg);
    formulations->add_formulation(
        std::make_shared<StubCatchmentFormulation>(catchment_id, formulation_response));

    auto features = std::make_shared<hy_features::HY_Features>(fabric, formulations);

    simulation_time_params st_params(
        "2015-12-01 00:00:00", "2015-12-01 02:00:00", output_interval_seconds);
    Simulation_Time sim_time(st_params);

    ngen::LayerDescription desc{
        "test layer", "s", 0, static_cast<double>(output_interval_seconds)};
    std::vector<std::string> processing_units{catchment_id};
    ngen::Layer layer(desc, processing_units, sim_time, *features, fabric, 0);

    std::vector<double> empty_doubles;
    std::unordered_map<std::string, int> empty_map;
    layer.update_models(
        boost::span<double>(empty_doubles),
        empty_map,
        boost::span<double>(empty_doubles),
        empty_map,
        0);

    auto nexus = features->nexus_at(nexus_id);
    auto upstream = nexus->inspect_upstream_flows(0);
    return upstream.first;
}

constexpr double kFlowTolerance = 1.0e-9;

} // namespace

// Scenario A: non-hourly (15-minute) output interval. The nexus must receive
// response * area_m2 / step, not response * area_m2 / 3600. This is the
// regression the fix targets.
TEST(LayerTimestepScalingTest, NexusFlowScalesByStep_A_NinetySecondStep)
{
    constexpr double response_m_per_step = 0.001;
    constexpr double area_sqkm = 2.5;
    constexpr int step_seconds = 900;

    const double actual = run_layer_step_and_get_nexus_flow(
        step_seconds, response_m_per_step, area_sqkm);
    const double expected =
        response_m_per_step * area_sqkm * 1.0e6 / static_cast<double>(step_seconds);

    EXPECT_NEAR(actual, expected, kFlowTolerance);
}

// Scenario B: hourly baseline. Catches a regression where future changes
// re-introduce a divisor that only happens to match 3600.
TEST(LayerTimestepScalingTest, NexusFlowScalesByStep_B_HourlyStep)
{
    constexpr double response_m_per_step = 0.001;
    constexpr double area_sqkm = 2.5;
    constexpr int step_seconds = 3600;

    const double actual = run_layer_step_and_get_nexus_flow(
        step_seconds, response_m_per_step, area_sqkm);
    const double expected =
        response_m_per_step * area_sqkm * 1.0e6 / static_cast<double>(step_seconds);

    EXPECT_NEAR(actual, expected, kFlowTolerance);
}

#endif // !NGEN_WITH_MPI
