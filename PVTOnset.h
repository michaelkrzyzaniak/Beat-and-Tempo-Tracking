/* PVT-owned onset front end. Copyright 2026 GNA Services Inc. MIT license. */
#ifndef PVT_ONSET_H
#define PVT_ONSET_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PVT_ONSET_SPECTRAL_FLUX = 0,
    PVT_ONSET_NEIGHBOR_FLUX = 1,
    PVT_ONSET_HIGH_FREQUENCY_FLUX = 2
} pvt_onset_method;

/* Sum positive magnitude changes. Inputs are equally sized, one-sided spectra
 * in increasing frequency order; bin zero (DC) is ignored. Compression and
 * noise-floor removal belong to the caller. NEIGHBOR_FLUX compares against
 * the maximum of the previous bin and its immediate neighbors, suppressing
 * one-bin pitch movement. HIGH_FREQUENCY_FLUX weights by bin/(bins-1).
 * No allocation, global state, or input mutation. Negative/non-finite bins
 * are treated as zero; invalid pointers, sizes, or methods return zero.
 * The caller must supply bins readable floats in each array and retain/copy
 * the previous frame only AFTER calling. Finite overflow saturates FLT_MAX.
 * This is an onset strength, not a beat event or a calibrated confidence. */
float pvt_onset_strength(const float* current, const float* previous,
                         size_t bins, pvt_onset_method method);
#ifdef __cplusplus
}
#endif
#endif
