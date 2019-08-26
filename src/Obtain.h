/*----------------------------------------------------------------------
         _          __________                              _,
     _.-(_)._     ."          ".      .--""--.          _.-{__}-._
   .'________'.   | .--------. |    .'        '.      .:-'`____`'-:.
  [____________] /` |________| `\  /   .'``'.   \    /_.-"`_  _`"-._\
  /  / .\/. \  \|  / / .\/. \ \  ||  .'/.\/.\'.  |  /`   / .\/. \   `\
  |  \__/\__/  |\_/  \__/\__/  \_/|  : |_/\_| ;  |  |    \__/\__/    |
  \            /  \            /   \ '.\    /.' / .-\                /-.
  /'._  --  _.'\  /'._  --  _.'\   /'. `'--'` .'\/   '._-.__--__.-_.'   \
 /_   `""""`   _\/_   `""""`   _\ /_  `-./\.-'  _\'.    `""""""""`    .'`\
(__/    '|    \ _)_|           |_)_/            \__)|        '       |   |
  |_____'|_____|   \__________/   |              |;`_________'________`;-'
   '----------'    '----------'   '--------------'`--------------------`
------------------------------------------------------------------------
  ___  _    _        _        _
 / _ \| |__| |_ __ _(_)_ _   | |_
| (_) | '_ \  _/ _` | | ' \ _| ' \
 \___/|_.__/\__\__,_|_|_||_(_)_||_|
 
------------------------------------------------------------------------

  Made by Michael Krzyzaniak
 
   Notes. Went from about 30% realtime to about 16% realtime when switching from slowsin to fastsin.
 
  Version:
    1.0

----------------------------------------------------------------------*/
#ifndef __OBTAIN__
#define __OBTAIN__ 1

#if defined(__cplusplus)
extern "C"{
#endif   //(__cplusplus)

#include "DFT.h"

/*--------------------------------------------------------------------*/
typedef enum
{
  OBTAIN_ONSET_TRACKING,
  OBTAIN_ONSET_AND_TEMPO_TRACKING,
  OBTAIN_ONSET_AND_TEMPO_AND_BEAT_TRACKING,
}obtain_tracking_mode_t;

#define OBTAIN_SUGGESTED_SPECTRAL_FLUX_STFT_LEN         1024  //
#define OBTAIN_SUGGESTED_SPECTRAL_FLUX_STFT_OVERLAP     8     // hop size will be len / overlap
#define OBTAIN_SUGGESTED_OSS_FILTER_ORDER               15    // this will delay the signal by about 7 samples
#define OBTAIN_SUGGESTED_OSS_LENGTH                     1024  // enough to hold 3 seconds at 44.1kHz and hop of 128
#define OBTAIN_SUGGESTED_ONSET_THRESHOLD_N              1024  // size of moving average of oss to see if onset occured
#define OBTAIN_SUGGESTED_SAMPLE_RATE                    44100 //
#define OBTAIN_SUGGESTED_CBSS_LENGTH                    512   //should be at least a little bigger than the slowest tempo lag in oss samples

#define OBTAIN_DEFAULT_MIN_TEMPO                        60    //
#define OBTAIN_DEFAULT_MAX_TEMPO                        180   //
#define OBTAIN_DEFAULT_SPECTRAL_COMPRESSION_GAMMA       3     //
#define OBTAIN_DEFAULT_CORRELATION_EXPONENT             0.5   //
#define OBTAIN_DEFAULT_NUM_TEMPO_CANDIDATES             10    //
#define OBTAIN_DEFAULT_TRACKING_MODE                    OBTAIN_ONSET_AND_TEMPO_AND_BEAT_TRACKING
#define OBTAIN_DEFAULT_OSS_FILTER_CUTOFF                7     //
#define OBTAIN_DEFAULT_USE_AMP_NORMALIZATION            0     //
#define OBTAIN_DEFAULT_ONSET_TREHSHOLD                  1     //

#define OBTAIN_DEFAULT_NOISE_CANCELLATION_THRESHOLD     -74   //dB per freq bin

#define OBTAIN_DEFAULT_XCORR_NUM_PULSES                 8     //
#define OBTAIN_DEFAULT_XCORR_PULSE_LOCATIONS            {0, 1, 1.5, 2, 3, 4, 4.5, 6}
#define OBTAIN_DEFAULT_XCORR_PULSE_VALUES               {2.0, 1.0, 0.5, 1.5, 1.5, 0.5, 0.5, 0.5}
#define OBTAIN_DEFAULT_LOG_GAUSSIAN_TEMPO_WEIGHT_MEAN   100  //supress harmonic by favoring tempos closer to 100
#define OBTAIN_DEFAULT_LOG_GAUSSIAN_TEMPO_WEIGHT_WIDTH  100  //oss samples starndard deviation

#define OBTAIN_DEFAULT_GAUSSIAN_TEMPO_HISTOGRAM_DECAY   0.995  //
#define OBTAIN_DEFAULT_GAUSSIAN_TEMPO_HISTOGRAM_WIDTH   10    //oss samples starndard deviation

#define OBTAIN_DEFAULT_CBSS_ALPHA                       0.9   //90% old, 10 percent new
#define OBTAIN_DEFAULT_CBSS_ETA                         200   //width of gaussian window. Who knows what a sane value is.

typedef struct Opaque_Obtain_Struct Obtain;

typedef void (*obtain_onset_callback_t)             (void* SELF, unsigned long long sample_time);
typedef void (*obtain_tempo_callback_t)             (void* SELF, unsigned long long sample_time, double bpm, int beat_period_in_samples);
typedef void (*obtain_beat_callback_t)              (void* SELF, unsigned long long sample_time);

Obtain*   obtain_new                                (int spectral_flux_stft_len, int spectral_flux_stft_overlap, int oss_filter_order, int oss_length, int cbss_length, int onset_threshold_len, double sample_rate);

Obtain*   obtain_new_default                        ();
//Obtain*   obtain new_from_xml                     (char* path);

Obtain*   obtain_destroy                            (Obtain* self);
void      obtain_process                            (Obtain* self, dft_sample_t* input, int num_samples);

double    obtain_get_sample_rate                    (Obtain* self);
void      obtain_set_use_amplitude_normalization    (Obtain* self, int use);
int       obtain_get_use_amplitude_normalization    (Obtain* self);

//Equation 1. gamma must be >= 0, where 0 indicates no compression and the paper does not specify what value was used
void      obtain_set_spectral_compression_gamma     (Obtain* self, double gamma);
double    obtain_get_spectral_compression_gamma     (Obtain* self);
void      obtain_set_oss_filter_cutoff              (Obtain* self, double Hz);
double    obtain_get_oss_filter_cutoff              (Obtain* self);

/* generalized autocorrelation is IFFT(|FFT(x)|^(exponent)) */
void      obtain_set_autocorrelation_exponent       (Obtain* self, double exponent);
double    obtain_get_autocorrelation_exponent       (Obtain* self);
void      obtain_set_min_tempo                      (Obtain* self, double min_tempo);
double    obtain_get_min_tempo                      (Obtain* self);
void      obtain_set_max_tempo                      (Obtain* self, double max_tempo);
double    obtain_get_max_tempo                      (Obtain* self);
void      obtain_set_onset_threshold                (Obtain* self, double num_std_devs);
double    obtain_get_onset_threshold                (Obtain* self);
void      obtain_set_num_tempo_candidates           (Obtain* self, int num_candidates);
int       obtain_get_num_tempo_candidates           (Obtain* self);

void      obtain_set_noise_cancellation_threshold   (Obtain* self, double dB /*probably negative*/);
double    obtain_get_noise_cancellation_threshold   (Obtain* self);
void      obtain_set_cbss_alpha                     (Obtain* self, double alpha);
double    obtain_get_cbss_alpha                     (Obtain* self);
void      obtain_set_cbss_eta                       (Obtain* self, double eta);
double    obtain_get_cbss_eta                       (Obtain* self);
void      obtain_set_gaussian_tempo_histogram_decay (Obtain* self, double coefficient);
double    obtain_get_gaussian_tempo_histogram_decay (Obtain* self);
void      obtain_set_gaussian_tempo_histogram_width (Obtain* self, double width);
double    obtain_get_gaussian_tempo_histogram_width (Obtain* self);
void      obtain_set_log_gaussian_tempo_weight_mean (Obtain* self, double bpm);
double    obtain_get_log_gaussian_tempo_weight_mean (Obtain* self);
void      obtain_set_log_gaussian_tempo_weight_width(Obtain* self, double bpm);
double    obtain_get_log_gaussian_tempo_weight_width(Obtain* self);

void                      obtain_set_tracking_mode            (Obtain* self, obtain_tracking_mode_t mode);
obtain_tracking_mode_t    obtain_get_tracking_mode            (Obtain* self);
void                      obtain_set_onset_tracking_callback  (Obtain* self, obtain_onset_callback_t callback, void* callback_self);
obtain_onset_callback_t   obtain_get_onset_tracking_callback  (Obtain* self, void** returned_callback_self);
void                      obtain_set_tempo_tracking_callback  (Obtain* self, obtain_tempo_callback_t callback, void* callback_self);
obtain_tempo_callback_t   obtain_get_tempo_tracking_callback  (Obtain* self, void** returned_callback_self);
void                      obtain_set_beat_tracking_callback   (Obtain* self, obtain_beat_callback_t callback, void* callback_self);
obtain_beat_callback_t    obtain_get_beat_tracking_callback   (Obtain* self, void** returned_callback_self);

//get/set tempo, noise_cancellation thresh

#if defined(__cplusplus)
}
#endif   //(__cplusplus)

#endif   // __OBTAIN__
