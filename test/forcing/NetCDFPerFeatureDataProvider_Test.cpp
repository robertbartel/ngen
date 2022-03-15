#include <vector>
#include "gtest/gtest.h"
#include "NetCDFPerFeatureDataProvider.hpp"
#include "FileChecker.h"
#include <memory>
#include <vector>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <ctime>
#include <time.h>


using data_access::NetCDFPerFeatureDataProvider;

class NetCDFPerFeatureDataProviderTest : public ::testing::Test {

    protected:

    void SetUp() override;

    void TearDown() override;

    void setupForcing();

    std::shared_ptr<data_access::NetCDFPerFeatureDataProvider> nc_provider;

    typedef struct tm time_type;

    std::shared_ptr<time_type> start_date_time;

    std::shared_ptr<time_type> end_date_time;

};

void NetCDFPerFeatureDataProviderTest::SetUp() {
    //setupForcing();

    setupForcing();
}


void NetCDFPerFeatureDataProviderTest::TearDown() 
{

}

//Construct a forcing object
void NetCDFPerFeatureDataProviderTest::setupForcing()
{
    nc_provider = std::make_shared<data_access::NetCDFPerFeatureDataProvider>("/local/ngen/data/huc01/huc_01/forcing/netcdf/huc01.nc");
    start_date_time = std::make_shared<time_type>();
    end_date_time = std::make_shared<time_type>();
}

///Test AORC Forcing Object
TEST_F(NetCDFPerFeatureDataProviderTest, TestForcingDataRead)
{
    // check to see that the variable "T2D" exists
    auto var_names = nc_provider->get_avaliable_variable_names();
    auto pos = std::find(var_names.begin(), var_names.end(), "T2D");

    if ( pos != var_names.end() )
    {
        std::cout << "Found variable T2D\n";
    }
    else
    {
        std::cout << "The variable \"T2D\" is missing\n";
        FAIL();
    }

    auto start_time = nc_provider->get_data_start_time();
    auto ids = nc_provider->get_ids();
    auto duration = nc_provider->record_duration();

    double val1 = nc_provider->get_value(ids[0], "T2D", start_time, duration, "K", data_access::NetCDFPerFeatureDataProvider::MEAN);
    std::cout <<  val1 << "\n";

    EXPECT_NEAR(val1, 263.1, 0.00000612);

    double val2 = nc_provider->get_value(ids[0], "T2D", start_time, duration / 2, "K", data_access::NetCDFPerFeatureDataProvider::MEAN);
    std::cout <<  val2 << "\n";

    EXPECT_NEAR(val2, 263.1, 0.00000612);

    double val3 = nc_provider->get_value(ids[0], "T2D", start_time, duration * 4, "K", data_access::NetCDFPerFeatureDataProvider::MEAN);
    std::cout <<  val3 << "\n";

    EXPECT_NEAR(val3, 262.31, 0.00000612);
    
}
