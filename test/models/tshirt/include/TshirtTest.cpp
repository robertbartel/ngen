#include "gtest/gtest.h"
#include "tshirt/include/Tshirt.h"
#include "tshirt/include/tshirt_params.h"
#include "Pdm03.h"
#include "GIUH.hpp"
#include <fstream>
#include <string>
#include <cmath>
#include <vector>
#include <boost/tokenizer.hpp>
#include "FileChecker.h"

class TshirtModelTest : public ::testing::Test {

protected:

    TshirtModelTest() {

    }

    ~TshirtModelTest() override {

    }

    void SetUp() override;

    void TearDown() override;

    void setupArbitraryExampleCase();

};

void TshirtModelTest::SetUp() {

}

void TshirtModelTest::TearDown() {

}

namespace tshirt {
    /**
     * A custom extension of tshirt_model for testing, which do no ET calculation by overriding calc_evapotranspiration
     * to return zero loss.
     */
    class no_et_tshirt_model : public tshirt_model {
    public:
        no_et_tshirt_model(tshirt_params model_params, const shared_ptr<tshirt_state> &initial_state)
                : tshirt_model(model_params, initial_state) {}

        /**
         * An override of ET logic that does not perform any ET loss calculation, and thus returns zero loss.
         *
         * @param soil_m
         * @param et_params
         * @return 0.0, to represent zero calculated ET loss
         */
        double calc_evapotranspiration(double soil_m, shared_ptr<pdm03_struct> et_params) override {
            return 0.0;
        }

    };

}

// Simple test to make sure the run function executes and that the inherent mass-balance check returned by run is good.
TEST_F(TshirtModelTest, TestRun0) {

    double et_storage = 0.0;

    tshirt::tshirt_params params{1000.0, 1.0, 10.0, 0.1, 0.01, 3, 1.0, 1.0, 1.0, 1.0, 8, 1.0, 1.0, 100.0};
    double storage = 1.0;
    double input_flux = 10.0;

    // Testing with implied 0.0's state
    tshirt::tshirt_state state_1(1.0, 1.0);
    tshirt::tshirt_model model_1(params, make_shared<tshirt::tshirt_state>(state_1));
    shared_ptr<pdm03_struct> et_params_1 = make_shared<pdm03_struct>(pdm03_struct());
    int model_1_result = model_1.run(86400.0, input_flux, et_params_1);

    // Testing with explicit state of correct size
    tshirt::tshirt_state state_2(1.0, 1.0,
                                 vector<double> { 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0 });
    tshirt::tshirt_model model_2(params, make_shared<tshirt::tshirt_state>(state_2));
    shared_ptr<pdm03_struct> et_params_2 = make_shared<pdm03_struct>(pdm03_struct());
    int model_2_result = model_2.run(86400.0, input_flux, et_params_2);

    // TODO: figure out how to test for bogus/mismatched nash_n and state nash vector size (without silent error)

    // Should return 0 if mass balance check was good
    EXPECT_EQ(model_1_result, 0);
    EXPECT_EQ(model_2_result, 0);

}

/**
 * Test of actual Tshirt model execution, comparing against data generated with alternate, C-based implementation.
 */
TEST_F(TshirtModelTest, TestRun1) {

    std::vector<std::string> data_file_names = { "../test/data/model/tshirt/expected_results.csv",
                                                    "test/data/model/tshirt/expected_results.csv" };
    std::string data_file = utils::FileChecker::find_first_readable(data_file_names);

    ASSERT_FALSE(data_file.empty());

    std::ifstream data_in(data_file.c_str());

    ASSERT_TRUE(data_in.is_open());

    // Note using NGen value instead of Fred's value of 0.0 (which will cancel out things like Klf)
    //double mult = 1.0;
    double mult = 0.0;

    double maxsmc = 0.439;
    //double wltsmc = 0.055;
    double wltsmc = 0.066;
    double satdk = 3.38e-06;
    double satpsi = 0.355;
    double slop = 1.0;
    double bb = 4.05;
    double alpha_fc = 0.33;
    double expon = 6.0;

    // From Freds code; (approx) the average blue line drainage density for CONUS
    double drainage_density_km_per_km2 = 3.5;

    // NGen has 0.0000672
    // see lines 326-329 in Fred's code
    double Klf = 2.0 * 0.01 * mult * satdk * 2.0 * drainage_density_km_per_km2 * 3600;

    // NGen has 1.08
    // TODO: think this is what Fred's code uses (line 353) but verify
    // TODO: also, on line 351, Fred's code may need to be re-run with is_exponential = TRUE for valid test values
    double Cgw = 0.01;   // NGen has 1.08

    // Note that NGen uses 8
    int nash_n = 2;

    // Note that NGen uses 0.1
    double Kn = 0.03;

    // TODO: should this be 1.0 like Fred's, or 16.0 like Sugar Creek?
    double max_gw_storage = 16.0;

    //Define tshirt params
    //{maxsmc, wltsmc, satdk, satpsi, slope, b, multiplier, aplha_fx, klf, kn, nash_n, Cgw, expon, max_gw_storage}
    tshirt::tshirt_params tshirt_params{
            maxsmc,   //maxsmc FWRFH
            wltsmc,    //wltsmc
            satdk,   //satdk FWRFH
            satpsi,    //satpsi
            slop,   //slope
            bb,      //b bexp? FWRFH
            mult,    //multipier  FIXMME what should this value be
            alpha_fc,    //aplha_fc   FIXME what should this value be
            Klf,    //Klf F
            Kn,    //Kn Kn	0.001-0.03 F
            nash_n,      //nash_n     FIXME is 8 a good number for the cascade?
            Cgw,    //Cgw C? FWRFH
            expon,    //expon FWRFH
            max_gw_storage  //max_gw_storage Sgwmax FWRFH
    };

    // init gw res as half full for test
    double gw_storage = max_gw_storage * 0.5;

    // init soil reservoir as 2/3 full
    double soil_storage = tshirt_params.max_soil_storage_meters * 0.667;

    // init properly, with all having 0 storage at start
    std::vector<double> nash_storages(nash_n);
    for (unsigned int i = 0; i < nash_n; i++) {
        nash_storages[i] = 0.0;
    }

    std::shared_ptr<tshirt::tshirt_state> initial_state = std::make_shared<tshirt::tshirt_state>(
            tshirt::tshirt_state(soil_storage, gw_storage, nash_storages));

    // Use an implementation that doesn't do any ET loss calculations
    tshirt::tshirt_model model = tshirt::no_et_tshirt_model(tshirt_params, initial_state);

    std::shared_ptr<pdm03_struct> pdm_et_data = std::make_shared<pdm03_struct>(pdm03_struct());

    // Not really important at this time, since test uses a custom no-ET subclass, but have here anyway
    pdm_et_data->scaled_distribution_fn_shape_parameter = 1.3;
    pdm_et_data->vegetation_adjustment = 0.99;
    pdm_et_data->model_time_step = 0.0;
    pdm_et_data->max_height_soil_moisture_storerage_tank = 400.0;
    pdm_et_data->maximum_combined_contents = pdm_et_data->max_height_soil_moisture_storerage_tank /
                                            (1.0 + pdm_et_data->scaled_distribution_fn_shape_parameter);

    //double error_bound_percent = 0.000001;
    double error_bound_percent = 0.01;
    double error_upper_bound_min = 0.001;

    typedef boost::tokenizer<boost::escaped_list_separator<char>> Tokenizer;
    std::vector<std::string> result_vector;
    string line;

    while (getline(data_in,line)) {
        Tokenizer tokenizer(line);
        result_vector.assign(tokenizer.begin(), tokenizer.end());

        // vector now contains strings from one row, output to cout here
        copy(result_vector.begin(), result_vector.end(), ostream_iterator<string>(cout, "|"));

        //double hour = std::stod(result_vector[0]);
        double input_storage = std::stod(result_vector[1]);
        double expected_direct_runoff = std::stod(result_vector[2]);
        //double expected_giuh_runoff = std::stod(result_vector[3]);
        double expected_lateral_flow = std::stod(result_vector[4]);
        double expected_base_flow = std::stod(result_vector[5]);
        //double expected_total_discharge = std::stod(result_vector[6]);

        model.run(60 * 60, input_storage, pdm_et_data);

        double upper_bound_factor = 1.0 + error_bound_percent;
        double lower_bound_factor = 1.0 - error_bound_percent;

        double lateral_flow_upper_bound =
                expected_lateral_flow != 0.0 ? expected_lateral_flow * upper_bound_factor : error_upper_bound_min;
        double lateral_flow_lower_bound = expected_lateral_flow * lower_bound_factor;
        EXPECT_LE(model.get_fluxes()->soil_lateral_flow_meters_per_second, lateral_flow_upper_bound);
        //ASSERT_LE(model.get_fluxes()->soil_lateral_flow_meters_per_second, lateral_flow_upper_bound);
        EXPECT_GE(model.get_fluxes()->soil_lateral_flow_meters_per_second, lateral_flow_lower_bound);
        //ASSERT_GE(model.get_fluxes()->soil_lateral_flow_meters_per_second, lateral_flow_lower_bound);

        double base_flow_upper_bound =
                expected_base_flow != 0.0 ? expected_base_flow * upper_bound_factor : error_upper_bound_min;
        double base_flow_lower_bound = expected_base_flow * lower_bound_factor;
        /*
        //EXPECT_LE(model.get_fluxes()->groundwater_flow_meters_per_second, base_flow_upper_bound);
        ASSERT_LE(model.get_fluxes()->groundwater_flow_meters_per_second, base_flow_upper_bound);
        //EXPECT_GE(model.get_fluxes()->groundwater_flow_meters_per_second, base_flow_lower_bound);
        ASSERT_GE(model.get_fluxes()->groundwater_flow_meters_per_second, base_flow_lower_bound);
        */

        /*
        double direct_runoff_upper_bound =
                expected_direct_runoff != 0.0 ? expected_direct_runoff * upper_bound_factor : error_upper_bound_min;
        double direct_runoff_lower_bound = expected_direct_runoff * lower_bound_factor;
        //EXPECT_LE(model.get_fluxes()->surface_runoff_meters_per_second, direct_runoff_upper_bound);
        ASSERT_LE(model.get_fluxes()->surface_runoff_meters_per_second, direct_runoff_upper_bound);
        ASSERT_GE(model.get_fluxes()->surface_runoff_meters_per_second, direct_runoff_lower_bound);
        */
    }

}

