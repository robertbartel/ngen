#ifdef NGEN_BMI_C_LIB_TESTS_ACTIVE

#include "gtest/gtest.h"

#include <chrono>
#include <numeric>
#include <unordered_map>
#include <vector>

#include "FileChecker.h"
#include "Bmi_C_Static_Adapter.hpp"

class Bmi_C_Adapter_Static_Profiling_Test : public ::testing::Test {
protected:

    void SetUp() override;

    void TearDown() override;

    std::string file_search(const std::vector<std::string> &parent_dir_options, const std::string& file_basename);

    std::string config_file_name_0;
    std::string forcing_file_name_0;

    std::vector<std::string> expected_output_var_names = {
            "RAIN_RATE",
            "SCHAAKE_OUTPUT_RUNOFF",
            "GIUH_RUNOFF",
            "NASH_LATERAL_RUNOFF",
            "DEEP_GW_TO_CHANNEL_FLUX",
            "Q_OUT"
    };

    std::vector<std::string> expected_output_var_locations = {
            "node",
            "node",
            "node",
            "node",
            "node",
            "node"
    };

    std::vector<int> expected_output_var_grids = {
            0,
            0,
            0,
            0,
            0,
            0
    };

    std::vector<std::string> expected_output_var_units = {
            "m",
            "m",
            "m",
            "m",
            "m",
            "m"
    };

    std::vector<std::string> expected_output_var_types = {
            "double",
            "double",
            "double",
            "double",
            "double",
            "double"
    };

    int expected_grid_rank = 1;

    int expected_grid_size = 1;

    std::string expected_grid_type = "scalar";

    int expected_var_nbytes = 8; //type double

};

void Bmi_C_Adapter_Static_Profiling_Test::SetUp() {
    std::vector<std::string> config_path_options = {
            "test/data/bmi/c/cfe/",
            "./test/data/bmi/c/cfe/",
            "../test/data/bmi/c/cfe/",
            "../../test/data/bmi/c/cfe/",
    };
    std::string config_basename_0 = "cat_27_bmi_config.txt";
    config_file_name_0 = file_search(config_path_options, config_basename_0);

    std::vector<std::string> forcing_dir_opts = {"./data/forcing/", "../data/forcing/", "../../data/forcing/"};
    forcing_file_name_0 = file_search(forcing_dir_opts, "cat-27_2015-12-01 00_00_00_2015-12-30 23_00_00.csv");
}

void Bmi_C_Adapter_Static_Profiling_Test::TearDown() {

}

std::string
Bmi_C_Adapter_Static_Profiling_Test::file_search(const std::vector<std::string> &parent_dir_options, const std::string& file_basename) {
    // Build vector of names by building combinations of the path and basename options
    std::vector<std::string> name_combinations;

    // Build so that all path names are tried for given basename before trying a different basename option
    for (auto & path_option : parent_dir_options)
        name_combinations.push_back(path_option + file_basename);

    return utils::FileChecker::find_first_readable(name_combinations);
}


/** Profile the update function and GetValues functions */
TEST_F(Bmi_C_Adapter_Static_Profiling_Test, Profile)
{
    models::bmi::Bmi_C_Static_Adapter adapter("cfebmi.so", config_file_name_0, forcing_file_name_0, true, false,
                                              true, utils::StreamHandler());

    int num_ouput_items = adapter.GetOutputItemCount();
    int num_input_items = adapter.GetInputItemCount();

    std::vector<std::string> output_names = adapter.GetOutputVarNames();
    std::vector<std::string> input_names = adapter.GetInputVarNames();

    std::unordered_map<std::string, std::vector<long> > saved_times;


    using time_point = std::chrono::time_point<std::chrono::steady_clock>;
    auto to_micros = [](const time_point& s, const time_point& e){ return std::chrono::duration_cast<std::chrono::microseconds>(e - s).count();};

    adapter.Initialize();
    // Do the first few time steps
    for (int i = 0; i < 720; i++)
    {
        // record time for Update
        auto s1 = std::chrono::steady_clock::now();
        adapter.Update();
        auto e1 = std::chrono::steady_clock::now();
        saved_times["Update"].push_back(to_micros(s1,e1));

        // record time to get each output variable
        for(std::string name : output_names)
        {
            auto s2 = std::chrono::steady_clock::now();
            std::vector<double> values = adapter.GetValue<double>(name);
            auto e2 = std::chrono::steady_clock::now();
            saved_times["Get " + name].push_back(to_micros(s2,e2));
        }

        // record time to get each input variable
        for(std::string name : input_names)
        {
            auto s3 = std::chrono::steady_clock::now();
            std::vector<double> values = adapter.GetValue<double>(name);
            auto e3 = std::chrono::steady_clock::now();
            saved_times["Get " + name].push_back(to_micros(s3,e3));
        }

    }

    // calcuate average time for each record

    for ( auto& kv : saved_times )
    {
        auto key = kv.first;
        auto& v = kv.second;
        long sum = 0;

        sum = std::accumulate(v.begin(), v.end(), sum );
        double average = double(sum) / v.size();

        std::cout << "Average time for " << key << " = " << average << "µs\n";
    }

    adapter.Finalize();
}
#endif // NGEN_BMI_C_LIB_TESTS_ACTIVE