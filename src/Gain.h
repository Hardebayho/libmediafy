#pragma once

#include <mutex>

namespace libmediafy {
    /// Gain multiplier. Increase or decrease audio gain
    class Gain {
    public:
        Gain(float multiplier = 1.0f) : multiplier(multiplier) {}
        void set_multiplier(float mult) {
            std::unique_lock<std::mutex> lk{lock};
            multiplier = mult;
        }
        float get_multiplier() {
            std::unique_lock<std::mutex> lk{lock};
            return multiplier;
        }

        /// Size is the number of elements in the array. in and out must be the same size
        bool filter(float* in, int samples, int channels) {
            std::unique_lock<std::mutex> lk{lock};
            if (multiplier == 1.f) return true; // Nothing to do
            for (int i = 0; i < samples; i++) {
                for (int j = 0; j < channels; j++) {
                    in[j] *= multiplier;
                }
                in += channels;
            }
            return true;
        }
    private:
        std::mutex lock{};
        float multiplier;
    };
}