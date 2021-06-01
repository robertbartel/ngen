#ifndef NGEN_BMI_C_STATIC_ADAPTER_HPP
#define NGEN_BMI_C_STATIC_ADAPTER_HPP

#include <utility>
#include "Bmi_C_Adapter.hpp"

using namespace std;

// Declaring registration method from BMI model library
extern "C" {
    Bmi* register_bmi_cfe(Bmi *model);
};

namespace models {
    namespace bmi {

        class Bmi_C_Static_Adapter : public Bmi_C_Adapter {

        public:

            Bmi_C_Static_Adapter(string library_file_path, string config_file_name, string forcing_file_path,
                                 bool model_uses_forcing_file, bool allow_exceed_end, bool has_fixed_time_step,
                                 utils::StreamHandler output, bool call_bmi_initialize = true) : Bmi_C_Adapter(
                    move(library_file_path),
                    move(config_file_name),
                    move(forcing_file_path),
                    model_uses_forcing_file,
                    allow_exceed_end,
                    has_fixed_time_step,
                    "register_bmi_cfe",
                    output,
                    false) {
                Initialize();
            }


            /**
             * Initialize and load the backing model via the appropriate mechanism(s).
             *
             * For this implementation, the static library is assumed to be available and the necessary registration
             * function is forward declared.
             */
            void init_and_load_model() override {
                // Call registration function, which handles setting up this object's pointed-to member BMI struct
                register_bmi_cfe(bmi_model.get());
            }

        };
    }
}


#endif //NGEN_BMI_C_STATIC_ADAPTER_HPP
