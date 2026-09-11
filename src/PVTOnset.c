#include "../PVTOnset.h"
#include <float.h>
#include <math.h>

static double magnitude(float value) {
    return isfinite(value) && value > 0.0f ? (double)value : 0.0;
}

float pvt_onset_strength(const float* current, const float* previous,
                         size_t bins, pvt_onset_method method) {
    size_t i;
    double strength = 0.0;
    if (!current || !previous || bins < 2
        || method < PVT_ONSET_SPECTRAL_FLUX
        || method > PVT_ONSET_HIGH_FREQUENCY_FLUX) return 0.0f;
    for (i = 1; i < bins; ++i) {
        double reference = magnitude(previous[i]);
        double difference;
        if (method == PVT_ONSET_NEIGHBOR_FLUX) {
            if (i > 1) reference = fmax(reference, magnitude(previous[i - 1]));
            if (i < bins - 1) reference = fmax(reference, magnitude(previous[i + 1]));
        }
        difference = fmax(0.0, magnitude(current[i]) - reference);
        if (method == PVT_ONSET_HIGH_FREQUENCY_FLUX)
            difference *= (double)i / (double)(bins - 1);
        strength += difference;
        if (strength >= FLT_MAX) return FLT_MAX;
    }
    return (float)strength;
}
