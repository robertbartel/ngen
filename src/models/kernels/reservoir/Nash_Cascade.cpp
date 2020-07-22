#include "Nash_Cascade.hpp"

namespace reservoir {

    /**
     * Construct a cascade of a given number of linear reservoirs (or the minimum or maximum allowed) using the
     * provided parameters for each, with each initially having empty storage.
     *
     * @param size The number of internal reservoirs in the cascade.
     * @param max_storage_m The maximum storage param for all the internal reservoirs (in meters).
     * @param Kn The Kn param for all the internal reservoirs.
     * @param activation_threshold_m The activation threshold (in meters) param for all the internal reservoirs.
     */
    Nash_Cascade::Nash_Cascade(const unsigned short size, double max_storage_m, double Kn,
                               double activation_threshold_m)
           : Nash_Cascade(size, std::vector<double>{0}, max_storage_m, Kn, activation_threshold_m) {}

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
    Nash_Cascade::Nash_Cascade(const unsigned short size, std::vector<double> initial_storages_m, double max_storage_m,
                               double Kn, double activation_threshold_m)
    {
        // For now, min size is 2, and max is max unsigned short value
        reservoirs = std::vector<Reservoir>(size < 2 ? 2 : size);

        // Warn but proceed if there are too many init storage values
        if (initial_storages_m.size() > reservoirs.size()) {
            cout << "WARNING: extra initial storage values passed when creating Nash Cascade object." << endl;
        }

        double initial_ith_res_storage;
        for (std::vector<double>::size_type i = 0; i < reservoirs.size(); ++i) {
            // Fall back to storage value of 0.0 if there is nothing for the index in the initial storages vector
            initial_ith_res_storage = i < initial_storages_m.size() ? initial_storages_m[i] : 0.0;

            //construct a single outlet linear reservoir
            reservoirs[i] = Reservoir(0.0, max_storage_m, initial_ith_res_storage, Kn, 1.0, 0.0,
                                                activation_threshold_m);
        }
    }

    /**
     * Calculate and return the flux response (in meters per second) to the provided influx and timestep, updating
     * the storage amounts for the internal reservoirs of the cascade during the calculations.
     *
     * @param input_flux_m_per_s The input flux to account for in meters per second.
     * @param timestep_s The size of the timestep in seconds.
     *
     * @return The calculated output flux response for the cascade in meters per second.
     */
    double Nash_Cascade::calc_response_m_per_s(double input_flux_m_per_s, unsigned int timestep_s) {
        // Cycle through lateral flow Nash cascade of reservoirs
        // loop essentially copied from Hymod logic, but with different variable names
        double res_excess_h20;
        for (auto & res : reservoirs) {
            // get response water velocity of nonlinear reservoir
            input_flux_m_per_s = res.response_meters_per_second(input_flux_m_per_s, (int) timestep_s, res_excess_h20);

            // TODO: confirm this is the correct thing to do when there is excess
            input_flux_m_per_s += res_excess_h20 / timestep_s;
        }
        // Finally, return the output from the last reservoir in the cascade, now stored in input_flux_m_per_s
        return input_flux_m_per_s;
    }

    /**
     * Get the cascade's output velocity, as determined by the last response calculation, in meters per second.
     *
     * @return The cascade's output velocity, as determined by the last response calculation, in meters per second.
     */
    double Nash_Cascade::get_output_velocity_m_per_s() {
        // Get the velocity of the (single) outlet for the last reservoir in the cascade
        return reservoirs[reservoirs.size() - 1].velocity_meters_per_second_for_outlet(0);
    }

    /**
     * Get the size, or number of internal reservoirs, for this Nash Cascade.
     *
     * @return The size, or number of internal reservoirs, for this Nash Cascade.
     */
    std::vector<double>::size_type Nash_Cascade::get_size() {
        return reservoirs.size();
    }

    /**
     * Get the current storage value in meters of the internal reservoir at the given index.
     *
     * Note that 0.0 is returned if the reservoir index outside the valid range.
     *
     * @param reservoir_index The index value for the reservoir of interest.
     * @return The current storage value in meters of the internal reservoir at the given index.
     */
    double Nash_Cascade::get_storage_for_reservoir_at_index_m(std::vector<double>::size_type reservoir_index) {
        // TODO: should probably log something here.
        if (reservoir_index < 0 || reservoir_index >= reservoirs.size()) {
            return 0.0;
        }
        else {
            return reservoirs[reservoir_index].get_storage_height_meters();
        }
    }
}
