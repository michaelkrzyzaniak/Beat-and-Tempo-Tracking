#include "Obtain.h"
#include "STFT.h"
#include "Filter.h"
#include "Statistics.h"

#include <stdlib.h> //calloc
#include <math.h>   //log
#include <string.h> //memset

#include <stdio.h> //fprintf (testing only)

void obtain_spectral_flux_stft_callback (void*   SELF, dft_sample_t* real, dft_sample_t* imag, int N);
void obtain_onset_tracking              (Obtain* self, dft_sample_t* real, dft_sample_t* imag, int N);
void obtain_tempo_tracking              (Obtain* self);
void obtain_beat_tracking               (Obtain* self);

#define at(i, k) ((i)*self->LMS_L)+(k)

/*--------------------------------------------------------------------*/
struct Opaque_Obtain_Struct
{
  STFT*              spectral_flux_stft;
  dft_sample_t*      prev_spectrum_magnitude;
  int                should_normalize_amplitude;
  double             spectral_compression_gamma;
  double             noise_cancellation_threshold;
  AdaptiveThreshold* onset_threshold;
  double             sample_rate;
  double             oss_sample_rate;
  Filter*            oss_filter;
  dft_sample_t*      oss;
  int                oss_index;
  int                oss_length;

  int                max_lag;
  int                min_lag;
  int                num_tempo_candidates;
  dft_sample_t*      autocorrelation_real;
  dft_sample_t*      autocorrelation_imag;
  double             autocorrelation_exponent;
  OnlineAverage*     tempo_score_variance;
  dft_sample_t*      gaussian_tempo_histogram;
  double             gaussian_tempo_decay;
  double             one_minus_gaussian_tempo_decay;
  double             gaussian_tempo_width;

  unsigned long long num_audio_samples_processed;
  unsigned long long num_oss_frames_processed;
  unsigned long long next_expected_beat_time;
  unsigned long long next_expected_half_beat_time;
  float              tempo_bpm;
  int                beat_period_oss_samples;
  int                beat_period_audio_samples;
  
  float*             cbss;
  int                cbss_length;
  int                cbss_index;
  float              cbss_alpha;
  float              one_minus_cbss_alpha;
  float              cbss_eta;
  OnlineRegression*  cbss_linear_predictor;
  float*             cbss_linearly_detrended;
  char*              LMS;
  int                LMS_L;
  int                LMS_gamma;
  int                beat_search_width;

  obtain_tracking_mode_t   tracking_mode;
  obtain_onset_callback_t  onset_callback;
  obtain_tempo_callback_t  tempo_callback;
  obtain_beat_callback_t   beat_callback;
  void* onset_callback_self;
  void* tempo_callback_self;
  void* beat_callback_self;
};

/*--------------------------------------------------------------------*/
Obtain* obtain_new(int spectral_flux_stft_len, int spectral_flux_stft_overlap, int oss_filter_order, int oss_length, int onset_threshold_len, int cbss_length, double sample_rate)
{
  Obtain* self = calloc(1, sizeof(*self));
  if(self != NULL)
    {
      self->spectral_flux_stft = stft_new(spectral_flux_stft_len, spectral_flux_stft_overlap, 0);
      if(self->spectral_flux_stft == NULL) return obtain_destroy(self);
    
      self->prev_spectrum_magnitude = calloc(spectral_flux_stft_len, sizeof(*self->prev_spectrum_magnitude));
      if(self->prev_spectrum_magnitude == NULL) return obtain_destroy(self);
    
      self->onset_threshold = adaptive_threshold_new(onset_threshold_len);
      if(self->onset_threshold == NULL) return obtain_destroy(self);
      adaptive_threshold_set_smoothing(self->onset_threshold, 0);
    
      self->sample_rate = sample_rate;
      self->oss_sample_rate = sample_rate / (spectral_flux_stft_len / spectral_flux_stft_overlap);
      self->oss_filter = filter_new(FILTER_LOW_PASS, OBTAIN_DEFAULT_OSS_FILTER_CUTOFF, oss_filter_order);
      if(self->oss_filter == NULL) return obtain_destroy(self);
      filter_set_sample_rate(self->oss_filter, self->oss_sample_rate);
      filter_set_window_type(self->oss_filter, FILTER_WINDOW_HAMMING);
    
      self->oss_length = oss_length;
      self->oss = calloc(self->oss_length, sizeof(*self->oss));
      if(self->oss == NULL) return obtain_destroy(self);
      self->oss_index = 0;

      self->autocorrelation_real = calloc(2*self->oss_length, sizeof(*self->autocorrelation_real));
      if(self->autocorrelation_real == NULL) return obtain_destroy(self);
      self->autocorrelation_imag = calloc(2*self->oss_length, sizeof(*self->autocorrelation_imag));
      if(self->autocorrelation_imag == NULL) return obtain_destroy(self);

      self->gaussian_tempo_histogram = calloc(self->oss_length, sizeof(*self->gaussian_tempo_histogram));
      if(self->gaussian_tempo_histogram == NULL) return obtain_destroy(self);
    
      self->cbss_length = cbss_length;
      self->cbss = calloc(self->cbss_length, sizeof(*self->cbss));
      if(self->cbss == NULL) return obtain_destroy(self);

      self->cbss_linearly_detrended = calloc(self->cbss_length, sizeof(*self->cbss_linearly_detrended));
      if(self->cbss_linearly_detrended == NULL) return obtain_destroy(self);

      self->LMS_L = ceil(self->cbss_length / 2 - 1);
      self->LMS = calloc(self->cbss_length * self->LMS_L, sizeof(*self->LMS));
      if(self->LMS == NULL) return obtain_destroy(self);
      self->LMS_gamma = self->LMS_L;
    
      self->tempo_score_variance = online_average_new();
      if(self->tempo_score_variance == NULL) return obtain_destroy(self);
    
      self->cbss_linear_predictor = online_regression_new();
      if(self->cbss_linear_predictor == NULL) return obtain_destroy(self);
    
      obtain_set_min_tempo                   (self, OBTAIN_DEFAULT_MIN_TEMPO);
      obtain_set_max_tempo                   (self, OBTAIN_DEFAULT_MAX_TEMPO);
      obtain_set_spectral_compression_gamma  (self, OBTAIN_DEFAULT_SPECTRAL_COMPRESSION_GAMMA);
      obtain_set_autocorrelation_exponent    (self, OBTAIN_DEFAULT_CORRELATION_EXPONENT);
      obtain_set_num_tempo_candidates        (self, OBTAIN_DEFAULT_NUM_TEMPO_CANDIDATES);
      obtain_set_tracking_mode               (self, OBTAIN_DEFAULT_TRACKING_MODE);
      obtain_set_oss_filter_cutoff           (self, OBTAIN_DEFAULT_OSS_FILTER_CUTOFF);
      obtain_set_use_amplitude_normalization (self, OBTAIN_DEFAULT_USE_AMP_NORMALIZATION);
      obtain_set_onset_threshold             (self, OBTAIN_DEFAULT_ONSET_TREHSHOLD);
    
      obtain_set_noise_cancellation_threshold(self, OBTAIN_DEFAULT_NOISE_CANCELLATION_THRESHOLD);
      obtain_set_cbss_alpha                  (self, OBTAIN_DEFAULT_CBSS_ALPHA);
      obtain_set_cbss_eta                    (self, OBTAIN_DEFAULT_CBSS_ETA);
      obtain_set_gaussian_tempo_decay        (self, OBTAIN_DEFAULT_GAUSSIAN_TEMPO_DECAY);
      obtain_set_gaussian_tempo_width        (self, OBTAIN_DEFAULT_GAUSSIAN_TEMPO_WIDTH);
      obtain_set_beat_search_width           (self, OBTAIN_DEFAULT_BEAT_SEARCH_WIDTH);

      obtain_set_onset_tracking_callback     (self, NULL, NULL);
      obtain_set_tempo_tracking_callback     (self, NULL, NULL);
      obtain_set_beat_tracking_callback      (self, NULL, NULL);
    }
  return self;
}

/*--------------------------------------------------------------------*/
Obtain*   obtain_new_default                      ()
{
  return obtain_new(OBTAIN_SUGGESTED_SPECTRAL_FLUX_STFT_LEN,
                    OBTAIN_SUGGESTED_SPECTRAL_FLUX_STFT_OVERLAP,
                    OBTAIN_SUGGESTED_OSS_FILTER_ORDER,
                    OBTAIN_SUGGESTED_OSS_LENGTH,
                    OBTAIN_SUGGESTED_ONSET_THRESHOLD_N,
                    OBTAIN_SUGGESTED_CBSS_LENGTH,
                    OBTAIN_SUGGESTED_SAMPLE_RATE);
}

/*--------------------------------------------------------------------*/
Obtain* obtain_destroy(Obtain* self)
{
  if(self != NULL)
    {
      if(self->prev_spectrum_magnitude != NULL)
        free(self->prev_spectrum_magnitude);

      if(self->oss != NULL)
        free(self->oss);
    
      if(self->autocorrelation_real != NULL)
        free(self->autocorrelation_real);
    
      if(self->autocorrelation_imag != NULL)
        free(self->autocorrelation_imag);
    
      if(self->gaussian_tempo_histogram != NULL)
        free(self->gaussian_tempo_histogram);
    
      if(self->cbss != NULL)
        free(self->cbss);

      if(self->cbss_linearly_detrended != NULL)
        free(self->cbss_linearly_detrended);

      if(self->LMS != NULL)
        free(self->LMS);
    
      stft_destroy(self->spectral_flux_stft);
      filter_destroy(self->oss_filter);
      adaptive_threshold_destroy(self->onset_threshold);
      online_average_destroy(self->tempo_score_variance);
      online_regression_destroy(self->cbss_linear_predictor);
    
      free(self);
    }
  return (Obtain*) NULL;
}

/*--------------------------------------------------------------------*/
void obtain_onset_tracking              (Obtain* self, dft_sample_t* real, dft_sample_t* imag, int N)
{
  int i;
  int n_over_2  = N >> 1;
  float flux = 0;
  
  //get spectrum magnitude, ignore DCC component in future calculations
  dft_rect_to_polar(real, imag, n_over_2);
  real[0] = 0;

  //Mormalize -- I'm not sure this is the correct approach here if we expect periods of silence
  if(self->should_normalize_amplitude)
    dft_normalize_magnitude(real, n_over_2);
  
  //perform log compression
  if(self->spectral_compression_gamma > 0)
    {
      double denominator = 1 / log(1 + self->spectral_compression_gamma);
      for(i=1; i<n_over_2; i++)
        real[i] = denominator * log(1+self->spectral_compression_gamma*real[i]);
    }
  
  //bins less than -74 dB are noise-cancelled
  for(i=1; i<n_over_2; i++)
    if(real[i] < self->noise_cancellation_threshold)
      real[i] = 0;
  
  //Normalize again
  if(self->should_normalize_amplitude)
    dft_normalize_magnitude(real, n_over_2);
  
  //Calculate flux and save spectrum
  for(i=1; i<n_over_2; i++)
    {
      flux += (real[i] > self->prev_spectrum_magnitude[i]) ? real[i] - self->prev_spectrum_magnitude[i] : 0;
      self->prev_spectrum_magnitude[i] = real[i];
    }
  
  //7HZ low-pass filter flux to obtaion OSS, delays oss by filter_order / 2 oss samples
  filter_process_data(self->oss_filter, &flux, 1);
  self->oss[self->oss_index] = flux;
  ++self->oss_index; self->oss_index %= self->oss_length;

  //call onset detected callback; technically the threshold filter is storing the oss signal again, using ~1kb redundant space
  if(flux > 0)
    if(adaptive_threshold_update(self->onset_threshold, flux) == 1)
      if(self->onset_callback != NULL)
        {
          unsigned long long t = self->num_audio_samples_processed;
          t -= (filter_get_order(self->oss_filter) / 2) * (self->sample_rate  / self->oss_sample_rate);
          self->onset_callback(self->onset_callback_self, t);
        }
}

/*--------------------------------------------------------------------*/
void obtain_tempo_tracking              (Obtain* self)
{
  int i, j;
  
  //perform autocorrelation
  memset(self->autocorrelation_real, 0, 2 * self->oss_length * sizeof(*self->autocorrelation_real));
  memset(self->autocorrelation_imag, 0, 2 * self->oss_length * sizeof(*self->autocorrelation_imag));
  for(i=0; i<self->oss_length; i++)
    self->autocorrelation_real[i] = self->oss[(self->oss_index+i) % self->oss_length];
  dft_real_generalized_autocorrelation  (self->autocorrelation_real, self->autocorrelation_imag, 2*self->oss_length, self->autocorrelation_exponent);

  //the oss signal only has silence
  if(self->autocorrelation_real[0] == 0)
    return;

  //enhance autocorreltaion
  for(i=self->min_lag; i<self->max_lag; i++)
    {
      int two_i = i<<1;
      int four_i = two_i<<1;
    
      if(two_i < self->oss_length)
        self->autocorrelation_real[i] += self->autocorrelation_real[two_i];
      if(four_i < self->oss_length)
        self->autocorrelation_real[i] += self->autocorrelation_real[four_i];
    }
  
  //pick peaks
  float old_derivative = 1, new_derivative;
  int candidate_tempo_lags[self->num_tempo_candidates];
  for(i=0; i<self->num_tempo_candidates; candidate_tempo_lags[i++]=self->min_lag);
  int min_lag_index = 0;
  for(i=self->min_lag; i<self->max_lag-1; i++)
    {
      new_derivative = self->autocorrelation_real[i+1] - self->autocorrelation_real[i];
      if((old_derivative > 0) && (new_derivative < 0))
        {
          if(self->autocorrelation_real[i] > self->autocorrelation_real[candidate_tempo_lags[min_lag_index]])
            {
              candidate_tempo_lags[min_lag_index] = i;
              min_lag_index = 0;
              for(j=1; j<self->num_tempo_candidates; j++)
                if(self->autocorrelation_real[candidate_tempo_lags[j]] < self->autocorrelation_real[candidate_tempo_lags[min_lag_index]])
                  min_lag_index = j;
            }
        }
      old_derivative = new_derivative;
    }

  //cross correlate with pulses and get score components
  int   score_max     [self->num_tempo_candidates];
  int   score_variance[self->num_tempo_candidates];
  float sum_of_score_max      = 0;
  float sum_of_score_variance = 0;
  int   num_pulses            = OBTAIN_DEFAULT_XCORR_NUM_PULSES;
  float pulse_locations[]     = OBTAIN_DEFAULT_XCORR_PULSE_LOCATIONS;
  float pulse_values[]        = OBTAIN_DEFAULT_XCORR_PULSE_VALUES;
  
  //7% realtime spent in this loop
  for(i=0; i<self->num_tempo_candidates; i++)
    {
      int phi;
      score_max[i] = 0;
      online_average_init(self->tempo_score_variance);
      for(phi=0; phi<candidate_tempo_lags[i]; phi++)
        {
          int pulse;
          float x_corr = 0;
          for(pulse=0; pulse<num_pulses; pulse++)
            {
              int index = phi + pulse_locations[pulse] * candidate_tempo_lags[i];
              if(index < self->oss_length)
                x_corr += self->oss[(index + self->oss_index) % self->oss_length] * pulse_values[pulse];
            }
          online_average_update(self->tempo_score_variance, x_corr);
          if(x_corr > score_max[i])
            score_max[i] = x_corr;
        }
      score_variance[i] = online_average_variance(self->tempo_score_variance);
      sum_of_score_max += score_max[i];
      sum_of_score_variance += score_variance[i];
    }
  
  if((sum_of_score_max == 0) || sum_of_score_variance == 0)
    return;
  
  //score candidates
  float score, max_score =- 1;
  int   index_of_max_score = 0;
  for(i=0; i<self->num_tempo_candidates; i++)
    {
      score = (score_max[i] / sum_of_score_max) + (score_variance[i] / sum_of_score_variance);
      if(score > max_score)
        {
          index_of_max_score = i;
          max_score = score;
        }
    }
  
  //this is way more likely to be because there were no peaks in the autocorrelation
  //(i.e. during silence) than because it was the the true tempo, so reject it.
  //in the future we might need to explicitely count non-zero samples in the OSS
  if(index_of_max_score == self->min_lag)
    return;
  
  //store gaussian histogram
  int index_of_max_gaussian = 0;
  float gaussian = 0;
  float two_sigma_squared = self->gaussian_tempo_width;
  
  for(i=0; i<self->oss_length; i++)
    {
      gaussian = (i-candidate_tempo_lags[index_of_max_score]);
      gaussian *= gaussian;
      gaussian = exp(-gaussian / two_sigma_squared);
      self->gaussian_tempo_histogram[i] *= self->gaussian_tempo_decay;
      self->gaussian_tempo_histogram[i] += gaussian * self->one_minus_gaussian_tempo_decay;
      if(self->gaussian_tempo_histogram[i] > self->gaussian_tempo_histogram[index_of_max_gaussian])
        index_of_max_gaussian = i;
    }

  self->tempo_bpm = (int)(60*self->oss_sample_rate / index_of_max_gaussian);
  self->beat_period_oss_samples = index_of_max_gaussian;
  self->beat_period_audio_samples = self->sample_rate * self->beat_period_oss_samples / self->oss_sample_rate;
}

/*--------------------------------------------------------------------*/
void obtain_beat_tracking               (Obtain* self)
{
  int i, k;
  
  int beat_was_detected = 0;
  
  //calculate cbss (equation 5)
  float phi         = 0;
  float max_phi     = 0;
  int   start_index = -2   * self->beat_period_oss_samples;
  int   end_index   = -0.5 * self->beat_period_oss_samples;
  int   oss_index = (self->oss_index + self->oss_length - 1) % self->oss_length;
  int   base_cbss_index = self->cbss_index + (2*self->cbss_length);
  int   cbss_index;
  
  if(start_index < -self->cbss_length)
    start_index = -self->cbss_length;
  if(end_index < -self->cbss_length)
    end_index = -self->cbss_length;
  
  for(i=start_index; i<end_index; i++)
    {
      cbss_index = (base_cbss_index + i) % self->cbss_length;
      phi = log(-(i/(float)self->beat_period_oss_samples));
      phi *= phi;
      phi *= self->cbss_eta;
      phi = exp(phi);
      phi *= self->cbss[cbss_index];
    
      if(phi > max_phi)
        max_phi = phi;
    }
  
  //equation 6
  self->cbss[self->cbss_index] = (self->one_minus_cbss_alpha * self->oss[oss_index]) + (self->cbss_alpha * max_phi);
  ++self->cbss_index; self->cbss_index %= self->cbss_length;
  
  
  //beat tracking method 1
  int time_to_next_beat = self->num_oss_frames_processed - self->next_expected_beat_time;
  if(time_to_next_beat > self->beat_search_width)
    beat_was_detected = 1;
  else if(abs(time_to_next_beat) <= self->beat_search_width)
    {
      //subtract out linear prediction (1% realtime when run every frame)
      online_regression_init(self->cbss_linear_predictor);
      for(i=0; i<self->cbss_length; i++)
        online_regression_update(self->cbss_linear_predictor, i, self->cbss[(i + self->cbss_index) % self->cbss_length]);
      float m = online_regression_slope          (self->cbss_linear_predictor);
      float b = online_regression_y_intercept    (self->cbss_linear_predictor);
      for(i=0; i<self->cbss_length; i++)
        self->cbss_linearly_detrended[i] = self->cbss[(i + self->cbss_index) % self->cbss_length] - (m*i+b);

      //  create scalogram (16% realtime when run on every frame)
      //  re-using gamma saves 16% realtime when gamma is 100 and 19% when gamma is 50, when this runs on every frame
      //  so we only recalculate gamma when we start searching for a new beat, then we save is as we continue to search
      int needs_gamma = (time_to_next_beat == -self->beat_search_width);
      int max_k       = (needs_gamma) ? self->LMS_L : self->LMS_gamma;
    
      memset(self->LMS, 1, self->cbss_length * self->LMS_L * sizeof(*self->LMS)); //anything other than 0;

      for(k=0; k<max_k; k++)
        {
           for(i=k+1; i<self->cbss_length-k-1; i++)
             {
               if((self->cbss_linearly_detrended[i] > self->cbss_linearly_detrended[i-(k+1)]) &&
                  (self->cbss_linearly_detrended[i] > self->cbss_linearly_detrended[i+(k+1)])  )
                 self->LMS[at(i, k)] = 0;
             }
          //it is faster by 10% realtime to do this as a second loop
           for(; i<self->cbss_length; i++)
             {
               if((self->cbss_linearly_detrended[i] > self->cbss_linearly_detrended[i-(k+1)]) &&
                  (self->cbss_linearly_detrended[i] >= self->cbss_linearly_detrended[self->cbss_length-1])  )
                 self->LMS[at(i, k)] = 0;
             }
        }
   
      //calculate gamma (argmin of row-wise sum);
      int sum;
      if(needs_gamma)
        {
          self->LMS_gamma = 0;
          float min_sum = 1000000;
          for(k=0; k<self->LMS_L; k++)
            {
              sum = 0;
              for(i=0; i<self->cbss_length; i++)
                sum += self->LMS[at(i, k)];
              if(sum < min_sum)
                {
                  min_sum = sum;
                  self->LMS_gamma = k+1;
                }
            }
        }

      sum = 0;
      for(k=0; k<self->LMS_gamma; k++)
        //looking in the second-to-last column is more stable;
        //sum += self->LMS[at(self->cbss_length-1, k)];
        sum += self->LMS[at(self->cbss_length-2, k)];
      if(sum == 0)
        beat_was_detected = 1;
    }//end beat tracking method 1
  
  //beat tracking method 2
  /*
  if(self->next_expected_half_beat_time == self->num_oss_frames_processed)
    {
      int   num_pulses            = OBTAIN_DEFAULT_XCORR_NUM_PULSES;
      float pulse_locations[]     = OBTAIN_DEFAULT_XCORR_PULSE_LOCATIONS;
      float pulse_values[]        = OBTAIN_DEFAULT_XCORR_PULSE_VALUES;
      //these might not be the most recent samples possible...
      int   phi_start             = 0;
      int   phi_end               = self->beat_period_oss_samples;
      if(phi_end < self->cbss_length) phi_end = self->cbss_length;
      //int   phi_end               = self->cbss_length - (pulse_locations[num_pulses-1] * self->beat_period_oss_samples);
      //int   phi_start             = phi_end - self->beat_period_oss_samples;
      //if(phi_start < 0) phi_start = 0;
      int   phi;
      int   max_phi               = phi_start;
      float max_xcorr             = -100000;

      for(phi=phi_start; phi<phi_end; phi++)
        {
          int pulse;
          float x_corr = 0;
          for(pulse=0; pulse<num_pulses; pulse++)
            {
              //the tempo might have changed since system 1 ran, so what the hell?
              int index = phi + pulse_locations[pulse] * self->beat_period_oss_samples;
              if(index < self->cbss_length)
                x_corr += self->cbss[(index + self->cbss_index) % self->cbss_length] * pulse_values[pulse];
            }
          if(x_corr > max_xcorr)
            {
              max_xcorr = x_corr;
              max_phi = phi;
            }
        }
      //what if this was more than 1 beat ago because of bounds on phi?
      double last_beat = (-self->oss_length + max_phi) / self->oss_sample_rate;
      self->next_expected_beat_time = self->num_oss_frames_processed + last_beat + self->beat_period_oss_samples;
    }
  */
  if(beat_was_detected)
    if(self->beat_callback != NULL)
      {
        unsigned long long t = self->num_audio_samples_processed;
        t -= (filter_get_order(self->oss_filter) / 2) * (self->sample_rate  / self->oss_sample_rate);
        self->beat_callback (self->beat_callback_self, t);
        fprintf(stderr, "current_time: %llu\texpected_time: %llu\tdiff: %i\ttempo: %f\r\n", self->num_oss_frames_processed, self->next_expected_beat_time, time_to_next_beat, self->tempo_bpm);
        self->next_expected_beat_time = self->num_oss_frames_processed + self->beat_period_oss_samples;
        self->next_expected_half_beat_time = self->num_oss_frames_processed + (self->beat_period_oss_samples / 2);
      }
}

/*--------------------------------------------------------------------*/
void obtain_spectral_flux_stft_callback(void* SELF, dft_sample_t* real, dft_sample_t* imag, int N)
{
  Obtain* self = SELF;

  if(self->tracking_mode >= OBTAIN_ONSET_TRACKING)
    obtain_onset_tracking (self, real, imag, N);
  
  ++self->num_oss_frames_processed;
  
  if(self->tracking_mode >= OBTAIN_ONSET_AND_TEMPO_TRACKING)
    if(self->num_oss_frames_processed >= self->oss_length)
      obtain_tempo_tracking(self);
  
  if(self->tracking_mode >= OBTAIN_ONSET_AND_TEMPO_AND_BEAT_TRACKING)
    if(self->beat_period_oss_samples > 0)
      obtain_beat_tracking(self);
}

/*--------------------------------------------------------------------*/
/* resynthesized samples, if any, returned in real_input */
void obtain_process(Obtain* self, dft_sample_t* input, int num_samples)
{
  stft_process(self->spectral_flux_stft, input, num_samples, obtain_spectral_flux_stft_callback, self);
  self->num_audio_samples_processed += num_samples;
}

/*--------------------------------------------------------------------*/
double    obtain_get_sample_rate                  (Obtain* self)
{
  return self->sample_rate;
}

/*--------------------------------------------------------------------*/
void      obtain_set_use_amplitude_normalization  (Obtain* self, int use)
{
  self->should_normalize_amplitude = use;
}

/*--------------------------------------------------------------------*/
int       obtain_get_use_amplitude_normalization  (Obtain* self)
{
  return self->should_normalize_amplitude;
}

/*--------------------------------------------------------------------*/
void      obtain_set_spectral_compression_gamma   (Obtain* self, double gamma)
{
  if(gamma < 0) gamma = 0;
  self->spectral_compression_gamma = gamma;
}

/*--------------------------------------------------------------------*/
double    obtain_get_spectral_compression_gamma   (Obtain* self)
{
  return self->spectral_compression_gamma;
}

/*--------------------------------------------------------------------*/
void      obtain_set_oss_filter_cutoff            (Obtain* self, double Hz)
{
  filter_set_cutoff(self->oss_filter, Hz);
}

/*--------------------------------------------------------------------*/
double    obtain_get_oss_filter_cutoff            (Obtain* self)
{
  return filter_get_cutoff(self->oss_filter);
}

/*--------------------------------------------------------------------*/
void      obtain_set_autocorrelation_exponent     (Obtain* self, double exponent)
{
  self->autocorrelation_exponent = exponent;
}

/*--------------------------------------------------------------------*/
double    obtain_get_autocorrelation_exponent     (Obtain* self)
{
  return self->autocorrelation_exponent;
}

/*--------------------------------------------------------------------*/
void      obtain_set_min_tempo                    (Obtain* self, double min_tempo)
{
  int max_lag = (60 * self->oss_sample_rate / min_tempo) + 1;
  if(max_lag > self->oss_length) max_lag = self->oss_length;
  self->max_lag = max_lag;
}

/*--------------------------------------------------------------------*/
double    obtain_get_min_tempo                    (Obtain* self)
{
  return 60 * self->oss_sample_rate / self->max_lag;
}

/*--------------------------------------------------------------------*/
void      obtain_set_max_tempo                    (Obtain* self, double max_tempo)
{
  int min_lag = 60 * self->oss_sample_rate / max_tempo;
  if(min_lag < 0) min_lag = 0;
  self->min_lag = min_lag;
}

/*--------------------------------------------------------------------*/
double    obtain_get_max_tempo                    (Obtain* self)
{
  return 60 * self->oss_sample_rate / self->min_lag;
}

/*--------------------------------------------------------------------*/
void      obtain_set_onset_threshold              (Obtain* self, double num_std_devs)
{
  adaptive_threshold_set_threshold(self->onset_threshold, num_std_devs);
}

/*--------------------------------------------------------------------*/
double    obtain_get_onset_threshold              (Obtain* self)
{
  return adaptive_threshold_threshold(self->onset_threshold);
}

/*--------------------------------------------------------------------*/
void      obtain_set_num_tempo_candidates         (Obtain* self, int num_candidates)
{
  if(num_candidates < 1)
    num_candidates = 1;
  self->num_tempo_candidates = num_candidates;
}

/*--------------------------------------------------------------------*/
int       obtain_get_num_tempo_candidates         (Obtain* self)
{
  return self->num_tempo_candidates;
}

/*--------------------------------------------------------------------*/
void      obtain_set_noise_cancellation_threshold (Obtain* self, double thresh)
{
  thresh = pow(10, thresh/20.0);
  self->noise_cancellation_threshold = thresh;
}

/*--------------------------------------------------------------------*/
double    obtain_get_noise_cancellation_threshold (Obtain* self)
{
  double result = self->noise_cancellation_threshold;
  result = 20 * log10(result);
  return result;
}

/*--------------------------------------------------------------------*/
void      obtain_set_cbss_alpha                   (Obtain* self, double alpha)
{
  if(alpha < 0) alpha = 0;
  if(alpha > 1) alpha = 1;
  self->cbss_alpha = alpha;
  self->one_minus_cbss_alpha = 1 - alpha;
}

/*--------------------------------------------------------------------*/
double    obtain_get_cbss_alpha                   (Obtain* self)
{
  return self->cbss_alpha;
}

/*--------------------------------------------------------------------*/
void      obtain_set_cbss_eta                     (Obtain* self, double eta)
{
  self->cbss_eta = -eta;
}

/*--------------------------------------------------------------------*/
double    obtain_get_cbss_eta                     (Obtain* self)
{
  return -self->cbss_eta;
}

/*--------------------------------------------------------------------*/
void      obtain_set_gaussian_tempo_decay         (Obtain* self, double coefficient)
{
  if(coefficient < 0) coefficient = 0;
  if(coefficient > 1) coefficient = 1;
  self->gaussian_tempo_decay = coefficient;
  self->one_minus_gaussian_tempo_decay = (1-coefficient);
}

/*--------------------------------------------------------------------*/
double    obtain_get_gaussian_tempo_decay         (Obtain* self)
{
  return self->gaussian_tempo_decay;
}

/*--------------------------------------------------------------------*/
void      obtain_set_gaussian_tempo_width         (Obtain* self, double width)
{
  self->gaussian_tempo_width = 2 * (width * width);
}

/*--------------------------------------------------------------------*/
double    obtain_get_gaussian_tempo_width         (Obtain* self)
{
  return sqrt(self->gaussian_tempo_width/2);
}

/*--------------------------------------------------------------------*/
void      obtain_set_beat_search_width            (Obtain* self, int width_oss_frames)
{
  self->beat_search_width = width_oss_frames;
}

/*--------------------------------------------------------------------*/
int       obtain_get_beat_search_width            (Obtain* self)
{
  return self->beat_search_width;
}

/*--------------------------------------------------------------------*/
void                      obtain_set_tracking_mode            (Obtain* self, obtain_tracking_mode_t mode)
{
  if(mode < OBTAIN_ONSET_TRACKING)
    mode = OBTAIN_ONSET_TRACKING;
  if(mode > OBTAIN_ONSET_AND_TEMPO_AND_BEAT_TRACKING)
    mode = OBTAIN_ONSET_AND_TEMPO_AND_BEAT_TRACKING;
  self->tracking_mode = mode;
}

/*--------------------------------------------------------------------*/
obtain_tracking_mode_t    obtain_get_tracking_mode            (Obtain* self)
{
  return self->tracking_mode;
}

/*--------------------------------------------------------------------*/
void                      obtain_set_onset_tracking_callback  (Obtain* self, obtain_onset_callback_t callback, void* callback_self)
{
  self->onset_callback = callback;
  self->onset_callback_self = callback_self;
}

/*--------------------------------------------------------------------*/
obtain_onset_callback_t   obtain_get_onset_tracking_callback  (Obtain* self, void** returned_callback_self)
{
  return self->onset_callback;
  *returned_callback_self = self->onset_callback_self;
}

/*--------------------------------------------------------------------*/
void                      obtain_set_tempo_tracking_callback  (Obtain* self, obtain_tempo_callback_t callback, void* callback_self)
{
  self->tempo_callback = callback;
  self->tempo_callback_self = callback_self;
}

/*--------------------------------------------------------------------*/
obtain_tempo_callback_t   obtain_get_tempo_tracking_callback  (Obtain* self, void** returned_callback_self)
{
  return self->tempo_callback;
  *returned_callback_self = self->tempo_callback_self;
}

/*--------------------------------------------------------------------*/
void                      obtain_set_beat_tracking_callback   (Obtain* self, obtain_beat_callback_t callback, void* callback_self)
{
  self->beat_callback = callback;
  self->beat_callback_self = callback_self;
}

/*--------------------------------------------------------------------*/
obtain_beat_callback_t    obtain_get_beat_tracking_callback   (Obtain* self, void** returned_callback_self)
{
  return self->beat_callback;
  *returned_callback_self = self->beat_callback_self;
}

