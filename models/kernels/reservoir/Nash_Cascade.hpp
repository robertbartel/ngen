#ifndef NGEN_NASH_CASCADE_H
#define NGEN_NASH_CASCADE_H

#include <vector>
#include "Reservoir.hpp"

using namespace std;

namespace reservoir {

    /**
     * Encapsulation of structure and behavior of a Nash Cascade of multiple linear reservoirs.
     */
    class Nash_Cascade {

    public:
        /**
         * Construct a cascade of a given number of linear reservoirs (or the minimum or maximum allowed) using the
         * provided parameters for each, with each initially having empty storage.
         *
         * @param size The number of internal reservoirs in the cascade.
         * @param max_storage_m The maximum storage param for all the internal reservoirs (in meters).
         * @param Kn The Kn param for all the internal reservoirs.
         * @param activation_threshold_m The activation threshold (in meters) param for all the internal reservoirs.
         */
        Nash_Cascade(unsigned short size, double max_storage_m, double Kn, double activation_threshold_m);

        /**
         * Construct a cascade of a given number of linear reservoirs (or the minimum or maximum allowed) using the
         * provided parameters for each and the provided collection of initial storage values.
         *
         * Constructor relies on the assumption that a value of `0.0` should be used if there is no corresponding initial
         * storage value for an internal reservoir.  Further, it will warn but proceed if the collection has more values
         * than necessary.
         *
         * @param size The number of internal reservoirs in the cascade.
         * @param initial_storages_m The ordered collection of initial storage values (in meters) for the reservoirs
         *                           (implied as 0.0 if missing).
         * @param max_storage_m The maximum storage param for all the internal reservoirs (in meters).
         * @param Kn The Kn param for all the internal reservoirs.
         * @param activation_threshold_m The activation threshold (in meters) param for all the internal reservoirs.
         */
        Nash_Cascade(unsigned short size, std::vector<double> initial_storages_m, double max_storage_m, double Kn,
                     double activation_threshold_m);

        /**
         * Calculate and return the flux response (in meters per second) to the provided influx and timestep, updating
         * the storage amounts for the internal reservoirs of the cascade during the calculations.
         *
         * @param input_flux_m_per_s The input flux to account for in meters per second.
         * @param timestep_s The size of the timestep in seconds.
         *
         * @return The calculated output flux response for the cascade in meters per second.
         */
        double calc_response_m_per_s(double input_flux_m_per_s, unsigned int timestep_s);

        /**
         * Get the cascade's output velocity, as determined by the last response calculation, in meters per second.
         *
         * @return The cascade's output velocity, as determined by the last response calculation, in meters per second.
         */
        double get_output_velocity_m_per_s();

        /**
         * Get the size, or number of internal reservoirs, for this Nash Cascade.
         *
         * @return The size, or number of internal reservoirs, for this Nash Cascade.
         */
        std::vector<double>::size_type get_size();

        /**
         * Get the current storage value in meters of the internal reservoir at the given index.
         *
         * @param reservoir_index The index value for the reservoir of interest.
         * @return The current storage value in meters of the internal reservoir at the given index.
         */
        double get_storage_for_reservoir_at_index_m(std::vector<double>::size_type reservoir_index);

    private:
        std::vector<Reservoir> reservoirs;
    };

}

#endif //NGEN_NASH_CASCADE_H
