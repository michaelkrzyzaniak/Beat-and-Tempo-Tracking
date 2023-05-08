//gcc *.c ../../src/*.c

#include "../../BTT.h"
#include <stdio.h>

void onset_detected_callback(void* SELF, unsigned long long sample_time);
void beat_detected_callback (void* SELF, unsigned long long sample_time);

/*--------------------------------------------------------------------*/
int main(int argc, char* argv[])
{
  BTT* btt = btt_new_default();
  if(btt == NULL){perror("unable to create btt object"); return -2;}
  
  fprintf(stderr, "btt_get_beat_period_audio_samples: %i\r\n", btt_get_beat_period_audio_samples(btt));
  fprintf(stderr, "btt_get_tempo_bpm: %f\r\n", btt_get_tempo_bpm(btt));
  fprintf(stderr, "btt_get_count_in_n: %i\r\n", btt_get_count_in_n(btt));
  fprintf(stderr, "btt_get_use_amplitude_normalization: %i\r\n", btt_get_use_amplitude_normalization(btt));
  fprintf(stderr, "btt_get_spectral_compression_gamma: %f\r\n", btt_get_spectral_compression_gamma(btt));
  fprintf(stderr, "btt_get_oss_filter_cutoff: %f\r\n", btt_get_oss_filter_cutoff(btt));
  fprintf(stderr, "btt_get_onset_threshold: %f\r\n", btt_get_onset_threshold(btt));
  fprintf(stderr, "btt_get_onset_threshold_min: %f\r\n", btt_get_onset_threshold_min(btt));
  fprintf(stderr, "btt_get_noise_cancellation_threshold: %f\r\n", btt_get_noise_cancellation_threshold(btt));

  fprintf(stderr, "btt_get_autocorrelation_exponent: %f\r\n", btt_get_autocorrelation_exponent(btt));
  fprintf(stderr, "btt_get_min_tempo: %f\r\n", btt_get_min_tempo(btt));
  fprintf(stderr, "btt_get_max_tempo: %f\r\n", btt_get_max_tempo(btt));
  
  fprintf(stderr, "btt_get_num_tempo_candidates: %i\r\n", btt_get_num_tempo_candidates(btt));
  fprintf(stderr, "btt_get_gaussian_tempo_histogram_decay: %f\r\n", btt_get_gaussian_tempo_histogram_decay(btt));
  fprintf(stderr, "btt_get_gaussian_tempo_histogram_width: %f\r\n", btt_get_gaussian_tempo_histogram_width(btt));
  fprintf(stderr, "btt_get_log_gaussian_tempo_weight_mean: %f\r\n", btt_get_log_gaussian_tempo_weight_mean(btt));
  fprintf(stderr, "btt_get_log_gaussian_tempo_weight_width: %f\r\n", btt_get_log_gaussian_tempo_weight_width(btt));

  fprintf(stderr, "btt_get_cbss_alpha: %f\r\n", btt_get_cbss_alpha(btt));
  fprintf(stderr, "btt_get_cbss_eta: %f\r\n", btt_get_cbss_eta(btt));
  
  fprintf(stderr, "btt_get_beat_prediction_adjustment: %i\r\n", btt_get_beat_prediction_adjustment(btt));
  fprintf(stderr, "btt_get_beat_prediction_adjustment_audio_samples: %i\r\n", btt_get_beat_prediction_adjustment_audio_samples(btt));
  fprintf(stderr, "btt_get_predicted_beat_trigger_index: %i\r\n", btt_get_predicted_beat_trigger_index(btt));
  
  fprintf(stderr, "btt_get_predicted_beat_gaussian_width: %f\r\n", btt_get_predicted_beat_gaussian_width(btt));
  fprintf(stderr, "btt_get_ignore_spurious_beats_duration: %f\r\n", btt_get_ignore_spurious_beats_duration(btt));
  
  fprintf(stderr, "btt_get_analysis_latency_onset_adjustment: %i\r\n", btt_get_analysis_latency_onset_adjustment(btt));
  fprintf(stderr, "btt_get_analysis_latency_beat_adjustment: %i\r\n", btt_get_analysis_latency_beat_adjustment(btt));
  
  fprintf(stderr, "btt_get_tracking_mode: %i\r\n", btt_get_tracking_mode(btt));
  fprintf(stderr, "btt_get_tracking_mode_string: %s\r\n", btt_get_tracking_mode_string(btt));
 
  btt = btt_destroy(btt);
}

/*--------------------------------------------------------------------*/
void onset_detected_callback(void* SELF, unsigned long long sample_time)
{

}

/*--------------------------------------------------------------------*/
void beat_detected_callback (void* SELF, unsigned long long sample_time)
{

}
