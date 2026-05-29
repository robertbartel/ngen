#include <vector>
#include "gtest/gtest.h"
#include "CsvPerFeatureForcingProvider.hpp"
#include "FileChecker.h"
#include <memory>
#include <vector>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <ctime>
#include <time.h>

class CsvPerFeatureForcingProviderTest : public ::testing::Test {

    protected:

    void SetUp() override;

    void TearDown() override;

    void setupForcing();

    std::shared_ptr<CsvPerFeatureForcingProvider> Forcing_Object;
    std::shared_ptr<CsvPerFeatureForcingProvider> Forcing_Object_2;
    std::shared_ptr<CsvPerFeatureForcingProvider> Forcing_Object_3; // explicit units

    typedef struct tm time_type;

    std::shared_ptr<time_type> start_date_time;

    std::shared_ptr<time_type> end_date_time;

};


void CsvPerFeatureForcingProviderTest::SetUp() {
    //setupForcing();

    setupForcing();
}


void CsvPerFeatureForcingProviderTest::TearDown() {
}


//Construct a forcing object
void CsvPerFeatureForcingProviderTest::setupForcing()
{
    std::vector<std::string> forcing_file_names = { 
        "test/data/forcing/cat-10_2015-12-01 00_00_00_2015-12-30 23_00_00.csv",
        "../test/data/forcing/cat-10_2015-12-01 00_00_00_2015-12-30 23_00_00.csv",
        "../../test/data/forcing/cat-10_2015-12-01 00_00_00_2015-12-30 23_00_00.csv"
        };
    std::string forcing_file_name = utils::FileChecker::find_first_readable(forcing_file_names);

    forcing_params forcing_p(forcing_file_name, "CsvPerFeature", "2015-12-14 21:00:00", "2015-12-30 23:00:00");

    Forcing_Object = std::make_shared<CsvPerFeatureForcingProvider>(forcing_p);

    forcing_file_names = { 
        "test/data/forcing/cat-27115-nwm-aorc-variant-derived-format.csv",
        "../test/data/forcing/cat-27115-nwm-aorc-variant-derived-format.csv",
        "../../test/data/forcing/cat-27115-nwm-aorc-variant-derived-format.csv"
        };
    forcing_file_name = utils::FileChecker::find_first_readable(forcing_file_names);

    forcing_params forcing_p_2(forcing_file_name, "CsvPerFeature", "2015-12-01 00:00:00", "2015-12-05 02:00:00");

    Forcing_Object_2 = std::make_shared<CsvPerFeatureForcingProvider>(forcing_p_2);

    forcing_file_names = { 
        "test/data/forcing/cat-27115-nwm-aorc-variant-derived-format-units.csv",
        "../test/data/forcing/cat-27115-nwm-aorc-variant-derived-format-units.csv",
        "../../test/data/forcing/cat-27115-nwm-aorc-variant-derived-format-units.csv"
    };
    forcing_file_name = utils::FileChecker::find_first_readable(forcing_file_names);

    forcing_params forcing_p_3(forcing_file_name, "CsvPerFeature", "2015-12-01 00:00:00", "2015-12-05 02:00:00");

    Forcing_Object_3 = std::make_shared<CsvPerFeatureForcingProvider>(forcing_p_3);
}

///Test AORC Forcing Object
TEST_F(CsvPerFeatureForcingProviderTest, TestForcingDataRead)
{
    double current_precipitation;

    time_t begin = Forcing_Object->get_data_start_time();

    int current_day_of_year;
    int i = 65;
    time_t t = begin+(i*3600);
    std::cerr << std::ctime(&t) << std::endl;

    current_precipitation = Forcing_Object->get_value(CatchmentAggrDataSelector("", CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, begin+(i*3600), 3600, ""), data_access::SUM);

    EXPECT_NEAR(current_precipitation, 7.9999999999999996e-07, 0.00000005);

    double temp_k = Forcing_Object->get_value(CatchmentAggrDataSelector("", CSDMS_STD_NAME_SURFACE_TEMP, begin+(i*3600), 3600, ""), data_access::MEAN);

    EXPECT_NEAR(temp_k, 286.9, 0.00001);

    int current_epoch;

    i = 387;
    current_precipitation = Forcing_Object->get_value(CatchmentAggrDataSelector("", CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, begin+(i*3600), 3600, ""), data_access::SUM);

    EXPECT_NEAR(current_precipitation, 6.9999999999999996e-07, 0.00000005);

    //Check exceeding the forcing range to retrieve the last forcing precipation rate
    i = 388;
    current_precipitation = Forcing_Object->get_value(CatchmentAggrDataSelector("", CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, begin+(i*3600), 3600, ""), data_access::SUM);

    EXPECT_NEAR(current_precipitation, 6.9999999999999996e-07, 0.00000005);
}

TEST_F(CsvPerFeatureForcingProviderTest, TestForcingDataReadAltFormat)
{
    double current_precipitation;

    time_t begin = Forcing_Object_2->get_data_start_time();

    int current_day_of_year;
    int i = 8;
    time_t t = begin+(i*3600);
    std::cerr << std::ctime(&t) << std::endl;

    current_precipitation = Forcing_Object_2->get_value(CatchmentAggrDataSelector("", CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, begin+(i*3600), 3600, ""), data_access::SUM);

    EXPECT_NEAR(current_precipitation, 0.00032685, 0.00000001);

    double temp_k = Forcing_Object_2->get_value(CatchmentAggrDataSelector("", CSDMS_STD_NAME_SURFACE_TEMP, begin+(i*3600), 3600, ""), data_access::MEAN);

    EXPECT_NEAR(temp_k, 265.77, 0.00001);

    int current_epoch;

    i = 34;
    current_precipitation = Forcing_Object_2->get_value(CatchmentAggrDataSelector("", CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, begin+(i*3600), 3600, ""), data_access::SUM);

    EXPECT_NEAR(current_precipitation, 0.00013539, 0.00000001);

}


TEST_F(CsvPerFeatureForcingProviderTest, TestForcingDataUnitConversion)
{
    double current_precipitation;

    time_t begin = Forcing_Object->get_data_start_time();

    int current_day_of_year;
    int i = 65;
    time_t t = begin+(i*3600);
    std::cerr << std::ctime(&t) << std::endl;

    current_precipitation = Forcing_Object->get_value(CatchmentAggrDataSelector("", CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, begin+(i*3600), 3600, ""), data_access::SUM);

    EXPECT_NEAR(current_precipitation, 7.9999999999999996e-07, 0.00000005);

    double temp_f = Forcing_Object->get_value(CatchmentAggrDataSelector("", CSDMS_STD_NAME_SURFACE_TEMP, begin+(i*3600), 3600, "degF"), data_access::MEAN);

    EXPECT_NEAR(temp_f, 56.749989014, 0.00001);

}

///Test AORC Forcing Object
TEST_F(CsvPerFeatureForcingProviderTest, TestGetAvailableForcingOutputs)
{
    auto afos = Forcing_Object->get_available_variable_names();
    EXPECT_EQ(afos.size(), 18);
    EXPECT_TRUE(std::find(afos.begin(), afos.end(), "DLWRF_surface") != afos.end());
    EXPECT_TRUE(std::find(afos.begin(), afos.end(), CSDMS_STD_NAME_SOLAR_LONGWAVE) != afos.end());

}

/// Resolve a fixture path against the same set of relative roots the rest of the suite uses.
static std::string find_fixture(const std::string& rel) {
    std::vector<std::string> candidates = {
        "test/data/forcing/" + rel,
        "../test/data/forcing/" + rel,
        "../../test/data/forcing/" + rel
    };
    return utils::FileChecker::find_first_readable(candidates);
}

/// Confirms record_duration() returns the fixture's actual row interval (900s), not the
/// historical hard-coded 3600.
TEST_F(CsvPerFeatureForcingProviderTest, TestRecordDurationFromNinetySecondCsv)
{
    forcing_params p(find_fixture("cat-fake-900s-interval.csv"),
                     "CsvPerFeature",
                     "2015-12-01 00:00:00",
                     "2015-12-01 01:45:00");
    CsvPerFeatureForcingProvider provider(p);

    EXPECT_EQ(provider.record_duration(), 900);
}

/// Asks for a 3600s-wide value window starting at the fixture's first row. With correct
/// 900s row scaling, get_value() must aggregate four rows (1+2+3+4 = 10e-6); under the
/// old hard-coded 3600s assumption it would aggregate only one row and return 1e-6.
TEST_F(CsvPerFeatureForcingProviderTest, TestPrecipSumOverHourSpansFourNinetySecondRows)
{
    forcing_params p(find_fixture("cat-fake-900s-interval.csv"),
                     "CsvPerFeature",
                     "2015-12-01 00:00:00",
                     "2015-12-01 01:45:00");
    CsvPerFeatureForcingProvider provider(p);
    const time_t begin = provider.get_data_start_time();

    const double aggregated = provider.get_value(
        CatchmentAggrDataSelector("", CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, begin, 3600, ""),
        data_access::SUM);

    EXPECT_NEAR(aggregated, 1.0e-5, 1e-12);
}

/// Single-row window: with 900s rows, get_value over duration 900 starting at the row
/// boundary returns exactly that row's stored value.
TEST_F(CsvPerFeatureForcingProviderTest, TestPrecipSingleNinetySecondRow)
{
    forcing_params p(find_fixture("cat-fake-900s-interval.csv"),
                     "CsvPerFeature",
                     "2015-12-01 00:00:00",
                     "2015-12-01 01:45:00");
    CsvPerFeatureForcingProvider provider(p);
    const time_t begin = provider.get_data_start_time();

    const double v = provider.get_value(
        CatchmentAggrDataSelector("", CSDMS_STD_NAME_LIQUID_EQ_PRECIP_RATE, begin, 900, ""),
        data_access::SUM);

    EXPECT_NEAR(v, 1.0e-6, 1e-12);
}

/// Non-uniform row spacing must be rejected at construction with a std::runtime_error
/// whose message identifies the offending row and the mismatch.
TEST_F(CsvPerFeatureForcingProviderTest, TestNonUniformIntervalThrows)
{
    forcing_params p(find_fixture("cat-fake-nonuniform-interval.csv"),
                     "CsvPerFeature",
                     "2015-12-01 00:00:00",
                     "2015-12-01 01:15:00");

    try {
        CsvPerFeatureForcingProvider provider(p);
        FAIL() << "expected std::runtime_error for non-uniform CSV intervals";
    } catch (const std::runtime_error& e) {
        const std::string msg = e.what();
        EXPECT_NE(msg.find("Non-uniform"), std::string::npos) << msg;
        EXPECT_NE(msg.find("row index"), std::string::npos) << msg;
        EXPECT_NE(msg.find("900"), std::string::npos) << msg;
        EXPECT_NE(msg.find("1800"), std::string::npos) << msg;
    }
}

///Test CSV Units Parsing
TEST_F(CsvPerFeatureForcingProviderTest, TestForcingUnitHeaderParsing)
{
    const time_t t = Forcing_Object_3->get_data_start_time() + (8 * 3600);
    const auto& varnames = this->Forcing_Object_3->get_available_variable_names();
    const std::vector<std::tuple<std::string, std::string, std::string>>& expected = {
        {"RAINRATE", "mm s^-1", "cm min^-1"},
        {"T2D", "K", "degC"},
        {"Q2D", "kg kg-1", "g kg-1"},
        {"U2D", "m s-1", "cm s-1"},
        {"V2D", "m/s", "cm s-1"},
        {"TEST", "kg", "g"},
        {"PSFC[Pa)", "Pa", "bar"},
        {"SWDOWN(W m-2]", "W m-2", "langley"},
        {"LWDOWN [alt]", "W m-2", "langley"}
    };

    for (auto ite = expected.begin(); ite != expected.end(); ite++) {
        const auto expected_name = std::get<0>(*ite);
        const auto expected_in_units = std::get<1>(*ite);
        const auto expected_out_units = std::get<2>(*ite);

        const double in_value = this->Forcing_Object_3->get_value(
            CatchmentAggrDataSelector("", expected_name, t, 3600, expected_in_units),
            data_access::SUM
        );
    
        const double out_value = this->Forcing_Object_3->get_value(
            CatchmentAggrDataSelector("", expected_name, t, 3600, expected_out_units),
            data_access::SUM
        );

        // make sure each expected column name is within varnames
        EXPECT_NE(std::find(varnames.begin(), varnames.end(), expected_name), varnames.end());
        
        // make sure units are correctly mapped
        if (ite - expected.begin() < 6) {
            // conversion expected
            EXPECT_NE(in_value, out_value);
        } else {
            // conversion is not expected, since there is no mapping
            EXPECT_NEAR(in_value, out_value, 0.00001);
        }
        
    }
}
