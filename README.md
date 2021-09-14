# Beat-and-Tempo-Tracking
Beat-and-Tempo-Tracking is an ANSI C library that taps its metaphorical foot along with the beat when it hears music. It is realtime and causal. It is an onset detector, tempo-estimator, and beat predictor. It is a combination of several state-of-the art methods. You feed buffers of audio data into it, and it notifies you when an onset occurs, when it has a new tempo estimate, or when it thinks the beat should happen. It requires no external libraries or packages, and has no platform-dependent code. It was designed to run on ebmedded linux computers in musical robots, and It should run on anything.

## Getting Started Code Snipit
```c
#include "BTT.h"

/*--------------------------------------------------------------------*/
int main(void)
{
  /* instantiate a new object */
  BTT* btt = btt_new_default();

  /* specify which functions should recieve notificaions */
  btt_set_onset_tracking_callback  (btt, onset_detected_callback, NULL);
  btt_set_beat_tracking_callback   (btt, beat_detected_callback , NULL);

  int buffer_size = 64;
  dft_sample_t buffer[buffer_size];
  
  for(;;)
  {
    /* Fill a buffer with your audio samples here then pass it to btt */
    btt_process(btt, buffer, buffer_size);
  }
}

/*--------------------------------------------------------------------*/
void onset_detected_callback(void* SELF, unsigned long long sample_time)
{
  //called when onset was detected
}

/*--------------------------------------------------------------------*/
void beat_detected_callback (void* SELF, unsigned long long sample_time)
{
  //called when beat was detected
}
```

## Constructor, Destructor, Processing Audio, and Configuration
##### Overview

##### New
```c
BTT*      btt_new                                (int spectral_flux_stft_len, int spectral_flux_stft_overlap,
                                                  int oss_filter_order      , int oss_length,
                                                  int cbss_length           , int onset_threshold_len, double sample_rate);
```
This function creates a new beat tracking object.
Args:
* spectral_flux_stft_len: the window size for the STFT used to calculate the onset signal, described in the "Onset Detection" section overview, below. Suggested value is BTT_SUGGESTED_SPECTRAL_FLUX_STFT_LEN (1024 audio samples)
* spectral_flux_stft_overlap: The overlap factor for the STFT.  Suggested value is  BTT_SUGGESTED_SPECTRAL_FLUX_STFT_OVERLAP (8, i.e. 12.5% overlap, or  a hop of 128 audio samples for the default window length)
* oss_filter_order the order of the low-pass filter that is applied to the oss, and whose function is described in the section entitled "OSS Filter Cutoff" below. Suggested value is BTT_SUGGESTED_OSS_FILTER_ORDER (15). This value should be odd.
* oss_length: the nuber of oss samples to accumulate for analysis. Suggested value is BTT_SUGGESTED_OSS_LENGTH (1024 samples).
* cbss_length: the nuber of cbss samples to accumulate for analysis. Suggested value is BTT_SUGGESTED_CBSS_LENGTH (1024 samples).
* onset_threshold_len: the size of the moving average described in the "Onset Threshold" section below. Suggested value is BTT_SUGGESTED_ONSET_THRESHOLD_N (1024 samples)
* sample_rate: the audio sample rate for audio samples passed to btt_process(). Suggested value is BTT_SUGGESTED_SAMPLE_RATE (44100 Hz)

Returns: A fully initalized BTT object.

Many of the arguments to this function interact in complex ways and affect the performance of the algorithm in ways that are difficult to predict, so you should usually use btt_new_default() unless you know what you are doing. 

##### New Default
```c
BTT*      btt_new_default                        ();
/* equivalent to */
BTT* btt = btt_new(BTT_SUGGESTED_SPECTRAL_FLUX_STFT_LEN,
                   BTT_SUGGESTED_SPECTRAL_FLUX_STFT_OVERLAP,
                   BTT_SUGGESTED_OSS_FILTER_ORDER,
                   BTT_SUGGESTED_OSS_LENGTH,
                   BTT_SUGGESTED_ONSET_THRESHOLD_N,
                   BTT_SUGGESTED_CBSS_LENGTH,
                   BTT_SUGGESTED_SAMPLE_RATE);
```
See btt_new above for more information.

##### Destroy
```c
BTT*      btt_destroy                            (BTT* self);
```
Free the BTT object and deallocate all of its internal resources. Returns (BTT*) NULL; It is good practice to assign to result to the object you are destroying, e.g.
btt = btt_destroy(btt);

##### Process
```c
void      btt_process                            (BTT* self, dft_sample_t* input, int num_samples);
```
Process audio data

##### Get Sample Rate
```c
double      btt_get_sample_rate                            (BTT* self);
```
Get the operating sample rate of the algorithm. This is read only and cannot be changed after calling btt_new();

##### Init
```c
void      btt_init                            (BTT* self,);
```
Put the algorithm into the same state it was in when it would  be in after instantiating it with btt_new_default();

##### Clear
```c
void      btt_clear                            (BTT* self);
```

Keep all of the current  parameter values but clear out the current tempo estimate, beat prediction, etc, so the next audio passed into btt_proceess() will be treated as if it were preceded by silence.

##### Tracking Modes
```c
typedef enum
{
  BTT_ONSET_TRACKING,
  BTT_ONSET_AND_TEMPO_TRACKING,
  BTT_ONSET_AND_TEMPO_AND_BEAT_TRACKING,
}btt_tracking_mode_t;

void                 btt_set_tracking_mode            (BTT* self, btt_tracking_mode_t mode);
btt_tracking_mode_t  btt_get_tracking_mode            (BTT* self);
/*default value: BTT_DEFAULT_TRACKING_MODE (BTT_ONSET_AND_TEMPO_AND_BEAT_TRACKING)*/
```
Set the tracking mode.  This is used to turn off parts of the algorithm that you don't need. Default is 

##### Callback Functions
```c
typedef void        (*btt_onset_callback_t)           (void* SELF, unsigned long long sample_time);
void                 btt_set_onset_tracking_callback  (BTT*  self, btt_onset_callback_t callback, void* callback_self);
btt_onset_callback_t btt_get_onset_tracking_callback  (BTT*  self, void** returned_callback_self); 

typedef void         (*btt_beat_callback_t)           (void* SELF, unsigned long long sample_time);
void                 btt_set_beat_tracking_callback   (BTT*  self, btt_beat_callback_t callback, void* callback_self);
btt_beat_callback_t  btt_get_beat_tracking_callback   (BTT*  self, void** returned_callback_self);
```
Too many pointers. Don't overthink it, just use the "getting started" code snippet or look at the demos if this dosen't make sense.

## Onset Detection
##### Overview
For onset detection, this library follows the method described here and similarly elsewhere:
http://www.ijsps.com/uploadfile/2017/1220/20171220034151817.pdf
A video showing what the onset parameter adustments do is here: 
https://youtu.be/w4Vp2QxSG9k

This library uses the spectral flux of the audio signal to detect onsets. It takes a windowed DFT of the audio, and in each window, it adds up all of the bins that have more energy than they did previously. This results in a signal, the 'onset signal (oss)' that should spike when there is a new note. This signal is low-pass filtered to remove noise. Then an onset is reported when the signal rises above a threshold that is a certain number of standard deviations over the running mean of the signal.

##### Noise Cancellation
```c
void      btt_set_noise_cancellation_threshold   (BTT* self, double dB /*probably negative*/);
double    btt_get_noise_cancellation_threshold   (BTT* self);
/*default value: BTT_DEFAULT_NOISE_CANCELLATION_THRESHOLD (-74 dB) */
```
For each window of audio, each bin whose value is less than the noise cancellation threshold will be set to 0;

##### Amplitude Normalization
```c
void      btt_set_use_amplitude_normalization    (BTT* self, int use);
int       btt_get_use_amplitude_normalization    (BTT* self);
/*default value: BTT_DEFAULT_USE_AMP_NORMALIZATION (false) */
```
Some papers suggest normalizing each window of audio before calculating flux. This dosen't make any sense and I wouldn't recommend doing it, but here are the functions to do it, so be my guest. If you turn amp normalization on, each window will be scaled so that the maxmium frequency bin is 1, after noise cancellation.
 
##### Spectral Compression Gamma
```c 
void      btt_set_spectral_compression_gamma     (BTT* self, double gamma);
double    btt_get_spectral_compression_gamma     (BTT* self);
/*default value: BTT_DEFAULT_SPECTRAL_COMPRESSION_GAMMA (0) */
```
For each window, the specturm is squashed down (compressed) logarithimcally using the formula:
COMPRESSED(spectrum) = log(1+gamma|spectrum|) / log(1+gamma)
Zero indicates no compression, and higher values of gamma have diminishing returns. I'm not sure it makes much difference in the onset detection, and setting it to 0 saves two expensive calls to log() per audio sample.

##### OSS Filter Cutoff
```c 
void      btt_set_oss_filter_cutoff              (BTT* self, double Hz);
double    btt_get_oss_filter_cutoff              (BTT* self);
/*default value: BTT_DEFAULT_OSS_FILTER_CUTOFF (10 Hz) */
```
The cutoff frequency of the low-pass filter applied to the spectral flux. You don't normally expect onsets more frequently than 10 or 15 Hz, so you might as well filter out everything above that. The filter order is an argument to btt_new(), and is constant for the life of the object.

##### Onset Threshold
```c 
void      btt_set_onset_threshold                (BTT* self, double num_std_devs);
double    btt_get_onset_threshold                (BTT* self);
/*default value: BTT_DEFAULT_ONSET_TREHSHOLD (1 standard deviation) */
```
Your onset callback will be called whenever the onset signal rises above the onset threshold. The threshold is adaptive. A moving average of the onset signal is calculated, and the threshold remaines the given number of standard deviations above the mean. The size of the moving average is an argument to btt_new, and is cconstant for the life of the object. This works well for percussion music and poorly for everything else. In the future I might try to do something better. Note that poor functioning of the onset detection does not affect the tempo estimates and beat tracking.

## Tempo Tracking
##### Overview
For tempo tracking, this library uses the method described in this paper:
http://webhome.csc.uvic.ca/~gtzan/output/taslp2014-tempo-gtzan.pdf

At ecah timestep, the oss is autocorrelated, and several of the highest peaks are taken to be candidate tempos. The candidates are scored by cross-correlating the the oss with ideal pulse trains at the respective tempo. The candidate with the highest score is considered to be the current local tempo estimate. The tempo estimator maintains a decaying histogram of tempo estiamtes. For each new estimate, a Gaussian spike whose mean is the new tempo estimate is added into the histogram, and the highest peak in the histogram is taken to be the current tempo of the music.

##### Clear
```c
double      btt_get_tempo_bpm                            (BTT* self);
```

Get the best estimate of the current tempo expressed in beats per minute

##### Clear
```c
int      btt_get_beat_period_audio_samples                            (BTT* self);
```

Get the best estimate of the current tempo expressed in the number of (mono) audio samples between consecutive beats.


##### Autocorrelation Exponent
```c 
void      btt_set_autocorrelation_exponent       (BTT* self, double exponent);
double    btt_get_autocorrelation_exponent       (BTT* self);
/*default value: BTT_DEFAULT_AUTOCORRELATION_EXPONENT (0.5) */
```
Generalized autocorrelation is performed on the oss, which is defined as GAC(oss) = IFFT(FFT(oss^exponent)). For regular correlation the exponent is 2, and the authors of the cited paper suggest a value of 0.5.

##### Min Tempo
```c 
void      btt_set_min_tempo                      (BTT* self, double min_tempo);
double    btt_get_min_tempo                      (BTT* self);
/*default value: BTT_DEFAULT_MIN_TEMPO (50 BPM) */
```
Only seach for tempos not less than the min tempo. Regardless of the value you set here, the min tempo will be hard bounded by the length of the oss buffer, which is an argument to oss_new and is constant for the life of the object (by default that would be about 0.3 Hz).

##### Max Tempo
```c 
void      btt_set_max_tempo                      (BTT* self, double max_tempo);
double    btt_get_max_tempo                      (BTT* self);
/*default value: BTT_DEFAULT_MAX_TEMPO (200 BPM) */
```
Only seach for tempos not greater than the max tempo. Regardless of the value you set here, the max tempo will be hard bounded by the oss sample rate, which is determined by the audio sample rate and the FFT hop size. These are arguments to oss_new and are constant for the life of the object (by default that would be about 20000 BPM).

##### Num Tempo Candidates
```c 
void      btt_set_num_tempo_candidates           (BTT* self, int num_candidates);
int       btt_get_num_tempo_candidates           (BTT* self);
/*default value: BTT_DEFAULT_NUM_TEMPO_CANDIDATES (10) */
```
At each time step, try this many tempo candidates -- i.e. this many peaks are picked out of the autocorrelation. Scoring candidate tempos is one of the most conputationally expensive parts of this algorithm. On my laptop, the whole algorithm runs in 20% of realtime, and 7% of realtime is spent scoring candidate tempos. Decreasing this will save computational time, at the possible expense of more spurious tempo readings.

##### Gaussian Tempo Histogram Decay
```c 
void      btt_set_gaussian_tempo_histogram_decay (BTT* self, double coefficient);
double    btt_get_gaussian_tempo_histogram_decay (BTT* self);
/*default value: BTT_DEFAULT_GAUSSIAN_TEMPO_HISTOGRAM_DECAY (0.999) */
```
At each oss sample, multiply the tempo histogram by this value. In the original paper, they use the value 1 (no decay), but that is an offline algorithm that is trying to score a single song with assumed constant tempo. Lowering this value will make tempo changes be recognized more quickly, but spurious tempo estimates can more easily take over.

##### Gaussian Tempo Histogram Width
```c 
void      btt_set_gaussian_tempo_histogram_width (BTT* self, double width);
double    btt_get_gaussian_tempo_histogram_width (BTT* self);
/*default value: BTT_DEFAULT_GAUSSIAN_TEMPO_HISTOGRAM_WIDTH (5 oss samples lag) */
```
The width of the gaussian spike to add into the tempo histogram for a new tempo estimate. 

##### Log Gaussian Tempo Weight Mean
```c 
void      btt_set_log_gaussian_tempo_weight_mean (BTT* self, double bpm);
double    btt_get_log_gaussian_tempo_weight_mean (BTT* self);
/*default value: BTT_DEFAULT_LOG_GAUSSIAN_TEMPO_WEIGHT_MEAN (100 BPM) */
```
The entire tempo histogram is weighted by a log-Gaussian window. This makes extreme tempos overall less likely than moderate ones. This helps the algorithm choose between harmonics of the tempo. This function sets the mean of this log-gaussian window.

##### Log Gaussian Tempo Weight Width
```c 
void      btt_set_log_gaussian_tempo_weight_width(BTT* self, double bpm);
double    btt_get_log_gaussian_tempo_weight_width(BTT* self);
/*default value: BTT_DEFAULT_LOG_GAUSSIAN_TEMPO_WEIGHT_WIDTH (+-75  BPM) */
```
The width of the log-Gaussian tempo histogram weight window.

## Beat Tracking
##### Overview
For beat tracking, this library uses the method described on page 60 of this paper, with small modifications for robustness:
https://qmro.qmul.ac.uk/xmlui/bitstream/handle/123456789/15050/adam_stark_phd_thesis_2011.pdf?sequence=1

This library calculates a quasi-periodic cumulative beat-strength signal (the cbss), which combines the onset signal with previous values of the cbss from 1 beat in the past.  This creates a signal that spikes every beat. The beat-tracker then cross-correlates the cbss with an impulse train contining 4 clicks, one beat apart. This strenghtens the peaks in the cbss and determines the location of the beats in time. Finally, at every time step, the location of the next beat is predicted by projecting forward the beat location by one beat. This involves adding a Gaussian spike into a buffer...  This is hard to explain. I'm going to make a video.

##### CBSS Alpha
```c 
void      btt_set_cbss_alpha                     (BTT* self, double alpha);
double    btt_get_cbss_alpha                     (BTT* self);
/*default value: BTT_DEFAULT_CBSS_ALPHA (0.9) */
```
##### CBSS Eta
```c 
void      btt_set_cbss_eta                       (BTT* self, double eta);
double    btt_get_cbss_eta                       (BTT* self);
/*default value: BTT_DEFAULT_CBSS_ETA (300) */
```
##### Beat Prediction Adjustment
```c 
void      btt_set_beat_prediction_adjustment     (BTT* self, int oss_samples_earlier);
int       btt_get_beat_prediction_adjustment     (BTT* self);
/*default value: BTT_DEFAULT_BEAT_PREDICTION_ADJUSTMENT (10) */
```
##### Predicted Beat Trigger Index
```c 
void      btt_set_predicted_beat_trigger_index   (BTT* self, int index);
int       btt_get_predicted_beat_trigger_index   (BTT* self);
/*default value: BTT_DEFAULT_PREDICTED_BEAT_TRIGGER_INDEX (20) */
```
##### Predicted Beat Gaussian Width
```c 
void      btt_set_predicted_beat_gaussian_width  (BTT* self, double width);
double    btt_get_predicted_beat_gaussian_width  (BTT* self);
/*default value: BTT_DEFAULT_PREDICTED_BEAT_GAUSSIAN_WIDTH (10 cbss samples) */
```
##### Ignore Spurious Beats Duration
```c 
void      btt_set_ignore_spurious_beats_duration (BTT* self, double percent_of_tempo);
double    btt_get_ignore_spurious_beats_duration (BTT* self);
/*default value: BTT_DEFAULT_IGNORE_SPURIOUS_BEATS_DURATION (40% of current beat) */
```

