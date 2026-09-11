#include "BTT.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); exit(1); } } while (0)
typedef struct { unsigned long long times[512]; size_t count; } Events;
static void record(void* context, unsigned long long time) {
    Events* events = context;
    CHECK(events->count < 512);
    CHECK(time < 44100ULL * 13);
    if(events->count) CHECK(time >= events->times[events->count - 1]);
    events->times[events->count++] = time;
}
static void run(int chunk, pvt_onset_method method, Events* onsets, Events* beats, int reset) {
    const int frames = 44100 * 12;
    float* samples = calloc((size_t)frames, sizeof(float));
    BTT* tracker = btt_new_default();
    int i;
    CHECK(samples && tracker);
    /* Exercise scratch-buffer replacement before the same timestamp comparison. */
    btt_set_num_tempo_candidates(tracker, 1);
    CHECK(btt_get_num_tempo_candidates(tracker) == 1);
    btt_set_num_tempo_candidates(tracker, BTT_DEFAULT_NUM_TEMPO_CANDIDATES);
    CHECK(btt_get_num_tempo_candidates(tracker) == BTT_DEFAULT_NUM_TEMPO_CANDIDATES);
    CHECK(btt_set_onset_detection_method(tracker, method));
    CHECK(btt_get_onset_detection_method(tracker) == method);
    CHECK(!btt_set_onset_detection_method(tracker, (pvt_onset_method)99));
    CHECK(btt_get_onset_detection_method(tracker) == method);
    btt_set_onset_tracking_callback(tracker, record, onsets);
    btt_set_beat_tracking_callback(tracker, record, beats);
    btt_set_onset_threshold_min(tracker, 0.01);
    for (i = 0; i < frames; ++i) {
        int phase = i % 22050;
        if (phase < 300) samples[i] = (float)(0.8 * exp(-phase / 80.0) * cos(phase * 0.71));
    }
    if(reset) {
        btt_process(tracker, samples, 88201); /* includes a partial STFT hop */
        btt_clear(tracker);
        onsets->count = beats->count = 0;
        CHECK(btt_get_onset_detection_method(tracker) == method);
    }
    for (i = 0; i < frames; i += chunk)
        btt_process(tracker, samples + i, frames - i < chunk ? frames - i : chunk);
    CHECK(isfinite(btt_get_tempo_bpm(tracker)));
    CHECK(onsets->count > 10);
    CHECK(beats->count > 5);
    btt_destroy(tracker);
    free(samples);
}
int main(void) {
    float zero[8] = {0}, low[8] = {0}, high[8] = {0}, shifted[8] = {0};
    int method;
    low[2] = high[6] = shifted[3] = 1.0f;
    CHECK(pvt_onset_strength(low, zero, 8, PVT_ONSET_SPECTRAL_FLUX) == 1.0f);
    CHECK(pvt_onset_strength(low, low, 8, PVT_ONSET_SPECTRAL_FLUX) == 0.0f);
    CHECK(pvt_onset_strength(shifted, low, 8, PVT_ONSET_NEIGHBOR_FLUX) == 0.0f);
    CHECK(pvt_onset_strength(shifted, low, 8, PVT_ONSET_SPECTRAL_FLUX) == 1.0f);
    CHECK(pvt_onset_strength(high, zero, 8, PVT_ONSET_HIGH_FREQUENCY_FLUX)
        > 2.9f * pvt_onset_strength(low, zero, 8, PVT_ONSET_HIGH_FREQUENCY_FLUX));
    CHECK(pvt_onset_strength(NULL, low, 8, PVT_ONSET_SPECTRAL_FLUX) == 0);
    CHECK(pvt_onset_strength(low, zero, 1, PVT_ONSET_SPECTRAL_FLUX) == 0);
    low[0] = FLT_MAX; low[1] = NAN; low[2] = INFINITY; low[3] = -1;
    CHECK(pvt_onset_strength(low, zero, 8, PVT_ONSET_SPECTRAL_FLUX) == 0);
    low[1] = low[2] = FLT_MAX;
    CHECK(pvt_onset_strength(low, zero, 8, PVT_ONSET_SPECTRAL_FLUX) == FLT_MAX);
    for(method = 0; method <= 2; ++method) {
        Events onset_a = {{0}, 0}, onset_b = {{0}, 0}, beat_a = {{0}, 0}, beat_b = {{0}, 0};
        run(1, (pvt_onset_method)method, &onset_a, &beat_a, 0);
        run(4093, (pvt_onset_method)method, &onset_b, &beat_b, 1);
        CHECK(onset_a.count == onset_b.count && beat_a.count == beat_b.count);
        CHECK(memcmp(onset_a.times, onset_b.times, onset_a.count * sizeof(*onset_a.times)) == 0);
        CHECK(memcmp(beat_a.times, beat_b.times, beat_a.count * sizeof(*beat_a.times)) == 0);
    }
    {
        BTT* tracker = btt_new_default();
        float silence[44100] = {0};
        CHECK(tracker);
        btt_set_tracking_mode(tracker, BTT_METRONOME_MODE);
        btt_set_metronome_bpm(tracker, 120);
        CHECK(fabs(btt_get_tempo_bpm(tracker) - 120) < 1);
        btt_process(tracker, silence, 44100); /* no callback installed */
        btt_process(NULL, silence, 1);
        btt_process(tracker, NULL, 1);
        btt_destroy(tracker);
    }
    puts("PVT onset methods and chunk-independent tracker timestamps passed");
    return 0;
}
