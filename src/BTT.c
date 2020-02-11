/*----------------------------------------------------------------------
 ____             _                     _
| __ )  ___  __ _| |_    __ _ _ __   __| |
|  _ \ / _ \/ _` | __|  / _` | '_ \ / _` |
| |_) |  __/ (_| | |_  | (_| | | | | (_| |
|____/ \___|\__,_|\__|  \__,_|_| |_|\__,_|
 
 _____
|_   _|__ _ __ ___  _ __   ___
  | |/ _ \ '_ ` _ \| '_ \ / _ \
  | |  __/ | | | | | |_) | (_) |
  |_|\___|_| |_| |_| .__/ \___/
                   |_|
 _____               _    _
|_   _| __ __ _  ___| | _(_)_ __   __ _
  | || '__/ _` |/ __| |/ / | '_ \ / _` |
  | || | | (_| | (__|   <| | | | | (_| |
  |_||_|  \__,_|\___|_|\_\_|_| |_|\__, |
                                  |___/
------------------------------------------------------------------------

  Made by Michael Krzyzaniak
 
  Version:
    1.0

----------------------------------------------------------------------*/
#include "BTT.h"
#include "STFT.h"
#include "Filter.h"
#include "Statistics.h"

#include <stdlib.h> //calloc
#include <math.h>   //log
#include <string.h> //memset

#define BPM_TO_LAG(bpm) (60 * self->oss_sample_rate / ((float)(bpm)))
#define LAG_TO_BPM(lag) (60 * self->oss_sample_rate / ((float)(lag)))

void btt_spectral_flux_stft_callback (void*   SELF, dft_sample_t* real, dft_sample_t* imag, int N);
void btt_onset_tracking              (BTT* self, dft_sample_t* real, dft_sample_t* imag, int N);
void btt_tempo_tracking              (BTT* self);
void btt_beat_tracking               (BTT* self);



#include <stdio.h> //fprintf (testing only)
#include "../../Main/extras/Network.h" //Testing Only
#include "../../Main/extras/OSC.h" //Testing Only
//#define DEBUG_ONSETS
#define DEBUG_TEMPO
//#define DEBUG_BEATS
//#define DEBUG_BEATS_2


/*--------------------------------------------------------------------*/
struct Opaque_BTT_Struct
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
  double             log_gaussian_tempo_weight_width;
  double             log_gaussian_tempo_weight_mean;
  dft_sample_t*      gaussian_tempo_histogram;
  double             gaussian_tempo_histogram_decay;
  double             one_minus_gaussian_tempo_histogram_decay;
  double             gaussian_tempo_histogram_width;

  unsigned long long num_audio_samples_processed;
  unsigned long long num_oss_frames_processed;
  int                beat_period_oss_samples;
  
  float*             cbss;
  int                cbss_length;
  int                cbss_index;
  double             cbss_alpha;
  double             one_minus_cbss_alpha;
  double             cbss_alpha_before_lock;
  double             cbss_eta;
  float*             predicted_beat_signal;
  int                predicted_beat_index;
  int                beat_prediction_adjustment;
  int                predicted_beat_trigger_index;
  int                predicted_beat_gaussian_width;
  unsigned long long ignore_beats_until;
  double             ignore_spurious_beats_duration;
  
  int                count_in_n;
  OnlineAverage*     count_in_average;
  int                count_in_count;
  unsigned long long last_count_in_time;
  
  btt_tracking_mode_t   tracking_mode;
  btt_tracking_mode_t   tracking_mode_before_count_in;
  btt_onset_callback_t  onset_callback;
  btt_beat_callback_t   beat_callback;
  void* onset_callback_self;
  void* tempo_callback_self;
  void* beat_callback_self;
  
  //testing stuff berlow here
    Network* net;
    char* osc_buff;
    int osc_buff_size;
};

/*--------------------------------------------------------------------*/
BTT*   btt_new_default                      ()
{
  return btt_new(BTT_SUGGESTED_SPECTRAL_FLUX_STFT_LEN,
                    BTT_SUGGESTED_SPECTRAL_FLUX_STFT_OVERLAP,
                    BTT_SUGGESTED_OSS_FILTER_ORDER,
                    BTT_SUGGESTED_OSS_LENGTH,
                    BTT_SUGGESTED_ONSET_THRESHOLD_N,
                    BTT_SUGGESTED_CBSS_LENGTH,
                    BTT_SUGGESTED_SAMPLE_RATE);
}

/*--------------------------------------------------------------------*/
BTT* btt_new(int spectral_flux_stft_len, int spectral_flux_stft_overlap, int oss_filter_order, int oss_length, int onset_threshold_len, int cbss_length, double sample_rate)
{
  BTT* self = calloc(1, sizeof(*self));
  if(self != NULL)
    {
      self->spectral_flux_stft = stft_new(spectral_flux_stft_len, spectral_flux_stft_overlap, 0);
      if(self->spectral_flux_stft == NULL) return btt_destroy(self);
    
      self->prev_spectrum_magnitude = calloc(spectral_flux_stft_len, sizeof(*self->prev_spectrum_magnitude));
      if(self->prev_spectrum_magnitude == NULL) return btt_destroy(self);
    
      self->onset_threshold = adaptive_threshold_new(onset_threshold_len);
      if(self->onset_threshold == NULL) return btt_destroy(self);
      adaptive_threshold_set_smoothing(self->onset_threshold, 0);
    
      self->sample_rate = sample_rate;
      self->oss_sample_rate = sample_rate / stft_get_hop(self->spectral_flux_stft);
      self->oss_filter = filter_new(FILTER_LOW_PASS, BTT_DEFAULT_OSS_FILTER_CUTOFF, oss_filter_order);
      if(self->oss_filter == NULL) return btt_destroy(self);
      filter_set_sample_rate(self->oss_filter, self->oss_sample_rate);
      filter_set_window_type(self->oss_filter, FILTER_WINDOW_HAMMING);
    
      self->oss_length = oss_length;
      self->oss = calloc(self->oss_length, sizeof(*self->oss));
      if(self->oss == NULL) return btt_destroy(self);
      self->oss_index = 0;

      self->autocorrelation_real = calloc(2*self->oss_length, sizeof(*self->autocorrelation_real));
      if(self->autocorrelation_real == NULL) return btt_destroy(self);
      self->autocorrelation_imag = calloc(2*self->oss_length, sizeof(*self->autocorrelation_imag));
      if(self->autocorrelation_imag == NULL) return btt_destroy(self);

      self->gaussian_tempo_histogram = calloc(self->oss_length, sizeof(*self->gaussian_tempo_histogram));
      if(self->gaussian_tempo_histogram == NULL) return btt_destroy(self);
    
      self->cbss_length = cbss_length;
      self->cbss = calloc(self->cbss_length, sizeof(*self->cbss));
      if(self->cbss == NULL) return btt_destroy(self);

      self->predicted_beat_signal = calloc(self->cbss_length, sizeof(*self->predicted_beat_signal));
      if(self->predicted_beat_signal == NULL) return btt_destroy(self);
      self->tempo_score_variance = online_average_new();
      if(self->tempo_score_variance == NULL) return btt_destroy(self);

      self->count_in_average = online_average_new();
      if(self->count_in_average == NULL) return btt_destroy(self);

      btt_init(self);
    
            //testing
        self->net = net_new();
        if(self->net == NULL) return btt_destroy(self);
        net_udp_connect(self->net, 9876);
  
        self->osc_buff_size = 8192; //why not?
        self->osc_buff = calloc(self->osc_buff_size, sizeof(*self->osc_buff));
        if(self->osc_buff == NULL) return btt_destroy(self);
    }
  return self;
}

/*--------------------------------------------------------------------*/
BTT* btt_destroy(BTT* self)
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

      if(self->predicted_beat_signal != NULL)
        free(self->predicted_beat_signal);
    
      stft_destroy(self->spectral_flux_stft);
      filter_destroy(self->oss_filter);
      adaptive_threshold_destroy(self->onset_threshold);
      online_average_destroy(self->tempo_score_variance);
      online_average_destroy(self->count_in_average);
    
      free(self);
    }
  return (BTT*) NULL;
}

/*--------------------------------------------------------------------*/
void      btt_init(BTT* self)
{
  btt_set_min_tempo                       (self, BTT_DEFAULT_MIN_TEMPO);
  btt_set_max_tempo                       (self, BTT_DEFAULT_MAX_TEMPO);
  btt_set_spectral_compression_gamma      (self, BTT_DEFAULT_SPECTRAL_COMPRESSION_GAMMA);
  btt_set_autocorrelation_exponent        (self, BTT_DEFAULT_AUTOCORRELATION_EXPONENT);
  btt_set_num_tempo_candidates            (self, BTT_DEFAULT_NUM_TEMPO_CANDIDATES);
  btt_set_tracking_mode                   (self, BTT_DEFAULT_TRACKING_MODE);
  btt_set_oss_filter_cutoff               (self, BTT_DEFAULT_OSS_FILTER_CUTOFF);
  btt_set_use_amplitude_normalization     (self, BTT_DEFAULT_USE_AMP_NORMALIZATION);
  btt_set_onset_threshold                 (self, BTT_DEFAULT_ONSET_TREHSHOLD);
  btt_set_onset_threshold_min             (self, BTT_DEFAULT_ONSET_TREHSHOLD_MIN);
  btt_set_noise_cancellation_threshold    (self, BTT_DEFAULT_NOISE_CANCELLATION_THRESHOLD);
  btt_set_cbss_alpha                      (self, BTT_DEFAULT_CBSS_ALPHA);
  btt_set_cbss_eta                        (self, BTT_DEFAULT_CBSS_ETA);
  btt_set_log_gaussian_tempo_weight_mean  (self, BTT_DEFAULT_LOG_GAUSSIAN_TEMPO_WEIGHT_MEAN);
  btt_set_log_gaussian_tempo_weight_width (self, BTT_DEFAULT_LOG_GAUSSIAN_TEMPO_WEIGHT_WIDTH);
  btt_set_gaussian_tempo_histogram_decay  (self, BTT_DEFAULT_GAUSSIAN_TEMPO_HISTOGRAM_DECAY);
  btt_set_gaussian_tempo_histogram_width  (self, BTT_DEFAULT_GAUSSIAN_TEMPO_HISTOGRAM_WIDTH);
  btt_set_beat_prediction_adjustment      (self, BTT_DEFAULT_BEAT_PREDICTION_ADJUSTMENT);
  btt_set_predicted_beat_trigger_index    (self, BTT_DEFAULT_PREDICTED_BEAT_TRIGGER_INDEX);
  btt_set_predicted_beat_gaussian_width   (self, BTT_DEFAULT_PREDICTED_BEAT_GAUSSIAN_WIDTH);
  btt_set_ignore_spurious_beats_duration  (self, BTT_DEFAULT_IGNORE_SPURIOUS_BEATS_DURATION);
  btt_set_count_in_n                      (self, BTT_DEFAULT_COUNT_IN_N);
  btt_set_onset_tracking_callback         (self, NULL, NULL);
  btt_set_beat_tracking_callback          (self, NULL, NULL);
  
  btt_clear(self);
}

/*--------------------------------------------------------------------*/
void      btt_clear(BTT* self)
{
  self->num_audio_samples_processed = 0;
  self->num_oss_frames_processed    = 0;
  self->oss_index                   = 0;
  self->count_in_count              = 0;
  online_average_init (self->count_in_average);
  memset(self->oss, 0, self->oss_length * sizeof(*self->oss));
  btt_init_tempo(self, 0);
}

/*--------------------------------------------------------------------*/
void      btt_init_tempo(BTT* self, double bpm /*0 to clear tempo*/)
{
  memset(self->gaussian_tempo_histogram, 0, self->oss_length * sizeof(*self->gaussian_tempo_histogram));
  memset(self->cbss, 0, self->cbss_length * sizeof(*self->cbss));
  memset(self->predicted_beat_signal, 0, self->cbss_length * sizeof(*self->predicted_beat_signal));
  
  self->beat_period_oss_samples = 0;
  self->cbss_index              = 0;
  
  if(bpm > 0)
    {
      float max_bpm = btt_get_max_tempo(self);
      float min_bpm = btt_get_min_tempo(self);
      while (bpm > max_bpm) bpm *= 0.5;
      while (bpm < min_bpm) bpm *= 2;
      int lag = round(BPM_TO_LAG(bpm));
    
      if((lag >= 0) && (lag < self->oss_length))
        {
         //normalize height later
          self->gaussian_tempo_histogram[lag] = 1;
          self->beat_period_oss_samples = lag;
        
          int i;
          int start = filter_get_order(self->oss_filter) / 2;
          for(i=self->cbss_length-1; i>=0; i-=lag)
          //for(i=2; i<self->cbss_length; i+=lag)
            self->cbss[(self->cbss_length + i) % self->cbss_length] = 15;
        }
      }
}
/*--------------------------------------------------------------------*/
int       btt_get_beat_period_audio_samples(BTT* self)
{
  return self->beat_period_oss_samples * stft_get_hop(self->spectral_flux_stft);
}
/*--------------------------------------------------------------------*/
double    btt_get_tempo_bpm(BTT* self)
{
  if(self->beat_period_oss_samples <=0)
    return 0;
  else
    return LAG_TO_BPM(self->beat_period_oss_samples);
}

/*--------------------------------------------------------------------*/
double    btt_get_tempo_certainty(BTT* self)
{
  return self->gaussian_tempo_histogram[self->beat_period_oss_samples];
}

/*--------------------------------------------------------------------*/
void btt_onset_tracking              (BTT* self, dft_sample_t* real, dft_sample_t* imag, int N)
{
  int i;
  int n_over_2  = N >> 1;
  float flux = 0;
  
  //get spectrum magnitude, ignore DCC component in future calculations
  dft_rect_to_polar(real, imag, n_over_2);
  real[0] = 0;
  
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
  
  //10HZ low-pass filter flux to obtaion OSS, delays oss by filter_order / 2 oss samples
  filter_process_data(self->oss_filter, &flux, 1);
  self->oss[self->oss_index] = flux;
  //++self->oss_index; self->oss_index %= self->oss_length;

  //call onset detected callback; technically the threshold filter is storing the oss signal again, using ~1kb redundant space
  if(flux > 0)
    if(adaptive_threshold_update(self->onset_threshold, flux) == 1)
      {
        if(self->onset_callback != NULL)
          {
            unsigned long long t = self->num_audio_samples_processed;
            t -= (filter_get_order(self->oss_filter) / 2) * stft_get_hop(self->spectral_flux_stft);
            self->onset_callback(self->onset_callback_self, t);
          }
      
        if(self->tracking_mode == BTT_COUNT_IN_TRACKING)
          {
            ++self->count_in_count;
            if(self->count_in_count > 1)
              online_average_update(self->count_in_average, self->num_oss_frames_processed - self->last_count_in_time);
            if(self->count_in_count >= self->count_in_n)
              {
                btt_init_tempo(self, LAG_TO_BPM(online_average_mean(self->count_in_average)));
                btt_set_tracking_mode(self, self->tracking_mode_before_count_in);
              }
            self->last_count_in_time = self->num_oss_frames_processed;
          }
      }
  
  /*testing*/
  #if defined DEBUG_ONSETS
    int num_bytes = oscConstruct(self->osc_buff, self->osc_buff_size, "oss", "fff", flux, adaptive_threshold_mean(self->onset_threshold), adaptive_threshold_threshold_value(self->onset_threshold));
    net_udp_send(self->net, self->osc_buff, num_bytes, "127.0.0.1", 7400);
  #endif
    /*end testing*/
    //move this back up later for clarity
    ++self->oss_index; self->oss_index %= self->oss_length;
}

/*--------------------------------------------------------------------*/
void btt_tempo_tracking              (BTT* self)
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
  
  //pick peaks from autocorrelation
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
  float score_max     [self->num_tempo_candidates];
  float score_variance[self->num_tempo_candidates];
  float sum_of_score_max      = 0;
  float sum_of_score_variance = 0;
  int   num_pulses            = BTT_DEFAULT_XCORR_NUM_PULSES;
  float pulse_locations[]     = BTT_DEFAULT_XCORR_PULSE_LOCATIONS;
  float pulse_values[]        = BTT_DEFAULT_XCORR_PULSE_VALUES;
  
  //7% realtime spent in this loop (dft-free cross correlation)
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
  
  //score candidates based on xcorr peak value and varience
  float score, max_score =- 1;
  int   index_of_max_score = 0;
  for(i=0; i<self->num_tempo_candidates; i++)
    {
      score = (score_max[i] / sum_of_score_max) + (score_variance[i] / sum_of_score_variance);
      if(score >= max_score)
        {
          index_of_max_score = i;
          max_score = score;
        }
    }

#if defined DEBUG_TEMPO
    //testing
    int num_bytes = oscConstruct(self->osc_buff, self->osc_buff_size, "n", "i", self->max_lag - self->min_lag);
    net_udp_send(self->net, self->osc_buff, num_bytes, "127.0.0.1", 7400);
    //end testing
  
    //fprintf(stderr, "%f\r\n", btt_get_tempo_certainty(self));
#endif

  //this is way more likely to be because there were no peaks in the autocorrelation
  //(i.e. during silence) than because it was the the true tempo, so reject it.
  //in the future we might need to explicitely count non-zero samples in the OSS
  if(index_of_max_score == self->min_lag)
    return;
  
  //store decaying gaussian histogram
  int index_of_max_gaussian = 0;
  float gaussian, log_gaussian;;
  log_gaussian = log2(candidate_tempo_lags[index_of_max_score] * self->log_gaussian_tempo_weight_mean);
  log_gaussian *= log_gaussian;
  log_gaussian = exp(-log_gaussian / self->log_gaussian_tempo_weight_width) * self->one_minus_gaussian_tempo_histogram_decay;

  for(i=self->min_lag; i<self->max_lag; i++)
    {
      gaussian = i-candidate_tempo_lags[index_of_max_score];
      gaussian *= gaussian;
      gaussian = exp(-gaussian / self->gaussian_tempo_histogram_width);

      self->gaussian_tempo_histogram[i] *= self->gaussian_tempo_histogram_decay;
      self->gaussian_tempo_histogram[i] += gaussian * log_gaussian;
      if(self->gaussian_tempo_histogram[i] > self->gaussian_tempo_histogram[index_of_max_gaussian])
        index_of_max_gaussian = i;
    
#if defined DEBUG_TEMPO
      //testing
      int num_bytes = oscConstruct(self->osc_buff, self->osc_buff_size, "t", "f", self->gaussian_tempo_histogram[i]);
      net_udp_send(self->net, self->osc_buff, num_bytes, "127.0.0.1", 7400);
      //end testing
#endif
    }
  
  self->beat_period_oss_samples = index_of_max_gaussian;
}

/*--------------------------------------------------------------------*/
void btt_beat_tracking               (BTT* self)
{
  int i;
  
  //calculate cbss
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
  
  //equation 5, search for score of previous beat
  for(i=start_index; i<end_index; i++)
    {
      cbss_index = (base_cbss_index + i) % self->cbss_length;
      phi = log2(-(i/(float)self->beat_period_oss_samples));
      phi *= phi;
      phi *= self->cbss_eta;
      phi = exp(phi);
      phi *= self->cbss[cbss_index];
    
      if(phi > max_phi)
        max_phi = phi;
    }
  
  //equation 6, blend score of previous beat with oss
  self->cbss[self->cbss_index] = (self->one_minus_cbss_alpha * self->oss[oss_index]) + (self->cbss_alpha * max_phi);
  
  #if defined DEBUG_BEATS_2
    //testing
    int num_bytes = oscConstruct(self->osc_buff, self->osc_buff_size, "cbs", "f",self->cbss[self->cbss_index]);
    net_udp_send(self->net, self->osc_buff, num_bytes, "127.0.0.1", 7400);
    //end testing
  #endif
  
  ++self->cbss_index; self->cbss_index %= self->cbss_length;
  
  #if defined DEBUG_BEATS_2
    //testing
    num_bytes = oscConstruct(self->osc_buff, self->osc_buff_size, "n", "i", self->beat_period_oss_samples);
    net_udp_send(self->net, self->osc_buff, num_bytes, "127.0.0.1", 7400);
    //end testing
  #endif
  
  //cross-correlate cbss with beat pulses
  int    num_pulses            = 4;;
  float  pulse_locations[]     = {0, 1, 2, 3, 4};
  float  pulse_values[]        = {1, 1, 1, 1};
  float* signal                = self->cbss;
  int    signal_length         = self->cbss_length;
  int    signal_index          = self->cbss_index;
  int    phase;
  int    max_phase = 0;
  float  val_of_max_phase = -1;
  
  for(phase=0; phase<self->beat_period_oss_samples; phase++)
    {
      int pulse;
      float x_corr = 0;
      for(pulse=0; pulse<num_pulses; pulse++)
        {
          int index = phase + pulse_locations[pulse] * self->beat_period_oss_samples;
          if(index < signal_length)
            x_corr += signal[(index + signal_index) % signal_length] * pulse_values[pulse];
        }
      if(x_corr > val_of_max_phase)
        {
          val_of_max_phase = x_corr;
          max_phase = phase;
        }
#if defined DEBUG_BEATS
        //testing
        if((self->num_oss_frames_processed % 10) == 0)
        {
          int num_bytes = oscConstruct(self->osc_buff, self->osc_buff_size, "x", "f", x_corr);
          net_udp_send(self->net, self->osc_buff, num_bytes, "127.0.0.1", 7400);
        }
        //end testing
#endif
    }
  
  //project beat into the future
  max_phase = max_phase - self->cbss_length + 1;
  max_phase -= self->beat_prediction_adjustment + filter_get_order(self->oss_filter) / 2;
  max_phase %= self->beat_period_oss_samples;
  max_phase = self->beat_period_oss_samples + max_phase;
  
  //add this into the probabilities of future beats
  float gaussian;
  int   index, max_index=0;
  float max_value=-1;
  for(i=0; i<self->cbss_length; i++)
    {
      gaussian = (i % self->beat_period_oss_samples)-max_phase;
      gaussian *= gaussian;
      gaussian = exp(-gaussian / self->predicted_beat_gaussian_width);
      index = (self->predicted_beat_index + i) % self->cbss_length;
      self->predicted_beat_signal[index] += gaussian;
      if(self->predicted_beat_signal[index] > max_value)
        {
          max_value = self->predicted_beat_signal[index];
          max_index = i;
        }
        fprintf(stderr, "HERE\r\n");
#if defined DEBUG_BEATS_2
        //testing
        if((self->num_oss_frames_processed % 10) == 0)
        {
           if(i < 400)
            {
            int num_bytes = oscConstruct(self->osc_buff, self->osc_buff_size, "g", "f", self->predicted_beat_signal[index]);
            net_udp_send(self->net, self->osc_buff, num_bytes, "127.0.0.1", 7400);
          }
        }
      //end testing
#endif
    }
  
  self->predicted_beat_signal[self->predicted_beat_index] = 0;
  ++self->predicted_beat_index; self->predicted_beat_index %= self->cbss_length;
  
  //pick peak and call callback
  if(max_index == self->predicted_beat_trigger_index)
    {
      if(self->num_oss_frames_processed >= self->ignore_beats_until)
        {
          self->ignore_beats_until = (int)(self->num_oss_frames_processed + (self->beat_period_oss_samples * self->ignore_spurious_beats_duration));
          if(self->beat_callback != NULL)
            {
              unsigned long long t = self->num_audio_samples_processed;
              t += (self->predicted_beat_trigger_index) * stft_get_hop(self->spectral_flux_stft);
              self->beat_callback (self->beat_callback_self, t);
            }
        }
    }
}

/*--------------------------------------------------------------------*/
void btt_spectral_flux_stft_callback(void* SELF, dft_sample_t* real, dft_sample_t* imag, int N)
{
  BTT* self = SELF;

  if(self->tracking_mode >= BTT_COUNT_IN_TRACKING)
    btt_onset_tracking (self, real, imag, N);
  
  ++self->num_oss_frames_processed;
  
  if((self->tracking_mode == BTT_ONSET_AND_TEMPO_TRACKING) ||
     (self->tracking_mode == BTT_ONSET_AND_TEMPO_AND_BEAT_TRACKING))
      if(self->num_oss_frames_processed >= self->oss_length)
        btt_tempo_tracking(self);

  if(self->tracking_mode >= BTT_ONSET_AND_TEMPO_AND_BEAT_TRACKING)
    if(self->beat_period_oss_samples > 0)
      btt_beat_tracking(self);
}

/*--------------------------------------------------------------------*/
/* resynthesized samples, if any, returned in real_input */
void btt_process(BTT* self, dft_sample_t* input, int num_samples)
{
  stft_process(self->spectral_flux_stft, input, num_samples, btt_spectral_flux_stft_callback, self);
  self->num_audio_samples_processed += num_samples;
}

/*--------------------------------------------------------------------*/
double    btt_get_sample_rate                  (BTT* self)
{
  return self->sample_rate;
}

/*--------------------------------------------------------------------*/
void      btt_set_use_amplitude_normalization  (BTT* self, int use)
{
  if(use < 0) use = 0;
  if(use > 1) use = 1;
  self->should_normalize_amplitude = use;
}

/*--------------------------------------------------------------------*/
int       btt_get_use_amplitude_normalization  (BTT* self)
{
  return self->should_normalize_amplitude;
}

/*--------------------------------------------------------------------*/
void      btt_set_spectral_compression_gamma   (BTT* self, double gamma)
{
  if(gamma < 0) gamma = 0;
  self->spectral_compression_gamma = gamma;
}

/*--------------------------------------------------------------------*/
double    btt_get_spectral_compression_gamma   (BTT* self)
{
  return self->spectral_compression_gamma;
}

/*--------------------------------------------------------------------*/
void      btt_set_oss_filter_cutoff            (BTT* self, double Hz)
{
  filter_set_cutoff(self->oss_filter, Hz);
}

/*--------------------------------------------------------------------*/
double    btt_get_oss_filter_cutoff            (BTT* self)
{
  return filter_get_cutoff(self->oss_filter);
}

/*--------------------------------------------------------------------*/
void      btt_set_autocorrelation_exponent     (BTT* self, double exponent)
{
  self->autocorrelation_exponent = exponent;
}

/*--------------------------------------------------------------------*/
double    btt_get_autocorrelation_exponent     (BTT* self)
{
  return self->autocorrelation_exponent;
}

/*--------------------------------------------------------------------*/
void      btt_set_min_tempo                    (BTT* self, double min_tempo)
{
  int max_lag = BPM_TO_LAG(min_tempo) + 1;
  if(max_lag < 0) max_lag = 0;
  
  if(max_lag > self->oss_length) max_lag = self->oss_length;
  self->max_lag = max_lag;
}

/*--------------------------------------------------------------------*/
double    btt_get_min_tempo                    (BTT* self)
{
  return LAG_TO_BPM(self->max_lag);
}

/*--------------------------------------------------------------------*/
void      btt_set_max_tempo                    (BTT* self, double max_tempo)
{
  int min_lag = BPM_TO_LAG(max_tempo);
  if(min_lag < 0) min_lag = 0;
  self->min_lag = min_lag;
}

/*--------------------------------------------------------------------*/
double    btt_get_max_tempo                    (BTT* self)
{
  return LAG_TO_BPM(self->min_lag);
}

/*--------------------------------------------------------------------*/
void      btt_set_onset_threshold              (BTT* self, double num_std_devs)
{
  adaptive_threshold_set_threshold(self->onset_threshold, num_std_devs);
}

/*--------------------------------------------------------------------*/
double    btt_get_onset_threshold              (BTT* self)
{
  return adaptive_threshold_threshold(self->onset_threshold);
}

/*--------------------------------------------------------------------*/
void      btt_set_onset_threshold_min            (BTT* self, double value)
{
  adaptive_threshold_set_threshold_min(self->onset_threshold, value);
}

/*--------------------------------------------------------------------*/
double    btt_get_onset_threshold_min            (BTT* self)
{
  return adaptive_threshold_threshold_min(self->onset_threshold);
}

/*--------------------------------------------------------------------*/
void      btt_set_num_tempo_candidates         (BTT* self, int num_candidates)
{
  if(num_candidates < 1)
    num_candidates = 1;
  self->num_tempo_candidates = num_candidates;
}

/*--------------------------------------------------------------------*/
int       btt_get_num_tempo_candidates         (BTT* self)
{
  return self->num_tempo_candidates;
}

/*--------------------------------------------------------------------*/
void      btt_set_noise_cancellation_threshold (BTT* self, double thresh)
{
  if(thresh > 0) thresh = 0;
  thresh = pow(10, thresh/20.0);
  self->noise_cancellation_threshold = thresh * stft_get_N (self->spectral_flux_stft);
}

/*--------------------------------------------------------------------*/
double    btt_get_noise_cancellation_threshold (BTT* self)
{
  double result = self->noise_cancellation_threshold / (double) stft_get_N (self->spectral_flux_stft);
  result = 20 * log10(result);
  return result;
}

/*--------------------------------------------------------------------*/
void      btt_set_cbss_alpha                   (BTT* self, double alpha)
{
  if(alpha < 0) alpha = 0;
  if(alpha > 1) alpha = 1;
  self->cbss_alpha = alpha;
  self->one_minus_cbss_alpha = 1 - alpha;
}

/*--------------------------------------------------------------------*/
double    btt_get_cbss_alpha                   (BTT* self)
{
  return self->cbss_alpha;
}

/*--------------------------------------------------------------------*/
void      btt_set_cbss_eta                     (BTT* self, double eta)
{
  if(eta < 0) eta = 0;
  self->cbss_eta = -eta;
}

/*--------------------------------------------------------------------*/
double    btt_get_cbss_eta                     (BTT* self)
{
  return -self->cbss_eta;
}

/*--------------------------------------------------------------------*/
void      btt_set_gaussian_tempo_histogram_decay         (BTT* self, double coefficient)
{
  if(coefficient < 0) coefficient = 0;
  if(coefficient > 0.999999) coefficient = 0.999999;
  self->gaussian_tempo_histogram_decay = coefficient;
  self->one_minus_gaussian_tempo_histogram_decay = 1 - coefficient;
}

/*--------------------------------------------------------------------*/
double    btt_get_gaussian_tempo_histogram_decay         (BTT* self)
{
  return self->gaussian_tempo_histogram_decay;
}

/*--------------------------------------------------------------------*/
void      btt_set_gaussian_tempo_histogram_width         (BTT* self, double width)
{
  if(width < 1) width = 1;
  self->gaussian_tempo_histogram_width = 2 * (width * width);
}

/*--------------------------------------------------------------------*/
double    btt_get_gaussian_tempo_histogram_width         (BTT* self)
{
  return sqrt(self->gaussian_tempo_histogram_width/2);
}

/*--------------------------------------------------------------------*/
void      btt_set_log_gaussian_tempo_weight_mean (BTT* self, double bpm)
{
  if(bpm < 0) bpm = 0;
  double width = btt_get_log_gaussian_tempo_weight_width(self);
  self->log_gaussian_tempo_weight_mean = 1.0 / BPM_TO_LAG(bpm);
  btt_set_log_gaussian_tempo_weight_width(self, width);
}

/*--------------------------------------------------------------------*/
double    btt_get_log_gaussian_tempo_weight_mean (BTT* self)
{
  return LAG_TO_BPM(1 / self->log_gaussian_tempo_weight_mean);
}

/*--------------------------------------------------------------------*/
void      btt_set_log_gaussian_tempo_weight_width(BTT* self, double bpm)
{
  if(bpm <= 0) return;
  
  double denominator = bpm / btt_get_log_gaussian_tempo_weight_mean(self);
  self->log_gaussian_tempo_weight_width = denominator * denominator;
}

/*--------------------------------------------------------------------*/
double    btt_get_log_gaussian_tempo_weight_width(BTT* self)
{
  return btt_get_log_gaussian_tempo_weight_mean(self) * sqrt(self->log_gaussian_tempo_weight_width);
}

/*--------------------------------------------------------------------*/
void      btt_set_beat_prediction_adjustment     (BTT* self, int oss_samples_earlier)
{
  self->beat_prediction_adjustment = oss_samples_earlier;
}

/*--------------------------------------------------------------------*/
int       btt_get_beat_prediction_adjustment     (BTT* self)
{
  return self->beat_prediction_adjustment;
}

/*--------------------------------------------------------------------*/
void      btt_set_predicted_beat_trigger_index   (BTT* self, int index)
{
  if(index < 0) index = 0;
  if(index >= self->cbss_length) index = self->cbss_length - 1;
  
  self->predicted_beat_trigger_index = index;
}

/*--------------------------------------------------------------------*/
int       btt_get_predicted_beat_trigger_index   (BTT* self)
{
  return self->predicted_beat_trigger_index;
}

/*--------------------------------------------------------------------*/
void      btt_set_predicted_beat_gaussian_width  (BTT* self, double width)
{
  if(width < 1) width = 1;
  self->predicted_beat_gaussian_width = 2 * width * width;
}

/*--------------------------------------------------------------------*/
double    btt_get_predicted_beat_gaussian_width  (BTT* self)
{
  return sqrt(self->predicted_beat_gaussian_width / 2);
}

/*--------------------------------------------------------------------*/
void      btt_set_ignore_spurious_beats_duration (BTT* self, double percent_of_tempo)
{
  if(percent_of_tempo < 0) percent_of_tempo = 0;
  self->ignore_spurious_beats_duration = percent_of_tempo * 0.01;
}

/*--------------------------------------------------------------------*/
double    btt_get_ignore_spurious_beats_duration (BTT* self)
{
  return self->ignore_spurious_beats_duration * 100;
}

/*--------------------------------------------------------------------*/
void       btt_set_count_in_n(BTT* self, int n)
{
  if(n < 1) n = 1;
  self->count_in_n = n;
}

/*--------------------------------------------------------------------*/
int      btt_get_count_in_n(BTT* self)
{
  return self->count_in_n;
}

/*--------------------------------------------------------------------*/
void                      btt_set_tracking_mode            (BTT* self, btt_tracking_mode_t mode)
{
  if(mode < 0)
    mode = 0;
  if(mode >= BTT_NUM_TRACKING_MODES)
    mode = BTT_NUM_TRACKING_MODES-1;
  
  if((self->tracking_mode == BTT_TEMPO_LOCKED_BEAT_TRACKING) && (mode != BTT_TEMPO_LOCKED_BEAT_TRACKING))
    btt_set_cbss_alpha(self, self->cbss_alpha_before_lock);
  if((mode == BTT_TEMPO_LOCKED_BEAT_TRACKING) && (self->tracking_mode != BTT_TEMPO_LOCKED_BEAT_TRACKING))
    {
      self->cbss_alpha_before_lock = btt_get_cbss_alpha(self);
      btt_set_cbss_alpha(self, 1);
    }
  if((mode == BTT_COUNT_IN_TRACKING) && (self->tracking_mode != BTT_COUNT_IN_TRACKING))
    {
      self->tracking_mode_before_count_in = self->tracking_mode;
      self->count_in_count              = 0;
      online_average_init (self->count_in_average);
    }
  
  self->tracking_mode = mode;
}

/*--------------------------------------------------------------------*/
btt_tracking_mode_t    btt_get_tracking_mode            (BTT* self)
{
  return self->tracking_mode;
}

/*--------------------------------------------------------------------*/
void                      btt_set_onset_tracking_callback  (BTT* self, btt_onset_callback_t callback, void* callback_self)
{
  self->onset_callback = callback;
  self->onset_callback_self = callback_self;
}

/*--------------------------------------------------------------------*/
btt_onset_callback_t   btt_get_onset_tracking_callback  (BTT* self, void** returned_callback_self)
{
  return self->onset_callback;
  *returned_callback_self = self->onset_callback_self;
}

/*--------------------------------------------------------------------*/
void                      btt_set_beat_tracking_callback   (BTT* self, btt_beat_callback_t callback, void* callback_self)
{
  self->beat_callback = callback;
  self->beat_callback_self = callback_self;
}

/*--------------------------------------------------------------------*/
btt_beat_callback_t    btt_get_beat_tracking_callback   (BTT* self, void** returned_callback_self)
{
  return self->beat_callback;
  *returned_callback_self = self->beat_callback_self;
}

/*--------------------------------------------------------------------*/
int  btt_get_beat_prediction_adjustment_audio_samples(BTT* self)
{
  return 0;
}
