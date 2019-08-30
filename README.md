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
  btt_set_tempo_tracking_callback  (btt, tempo_detected_callback, NULL);
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
void tempo_detected_callback (void* SELF, unsigned long long sample_time, double bpm, int beat_period_in_samples)
{
  //called periodically with tempo update
}

/*--------------------------------------------------------------------*/
void beat_detected_callback (void* SELF, unsigned long long sample_time)
{
  //called when beat was detected
}
```

## Constructor, Destructor, and Bookkeeping

## Onset Detection
This library uses the spectral flux of the audio signal to detect onsets. It takes a windowed DFT of the audio, and in each window, it adds up all of the bins that have more energy than they did previously. This results in a signal, the 'onset signal (oss)' that should spike when there is a new note. This signal is low-pass filtered to remove noise. Then an onset is reported when the signal rises above a threshold that is a certain number of standard deviations over the running mean of the signal.

```c
void      btt_set_noise_cancellation_threshold   (BTT* self, double dB /*probably negative*/);
double    btt_get_noise_cancellation_threshold   (BTT* self);
/*default value: BTT_DEFAULT_NOISE_CANCELLATION_THRESHOLD (-74 dB) */
```
For each window of audio, each bin whose value is less than the noise cancellation threshold will be set to 0;


```c
void      btt_set_use_amplitude_normalization    (BTT* self, int use);
int       btt_get_use_amplitude_normalization    (BTT* self);
/*default value: BTT_DEFAULT_USE_AMP_NORMALIZATION (false) */
```
Some papers suggest normalizing each window of audio before calculating flux. This dosen't make any sense and I wouldn't recomend doing it, but here are the functions to do it, so be my guest. If you turn amp normalization on, each window will be scaled so that the maxmium frequency bin is 1, after noise cancellation.
 

```c 
void      btt_set_spectral_compression_gamma     (BTT* self, double gamma);
double    btt_get_spectral_compression_gamma     (BTT* self);
/*default value: BTT_DEFAULT_SPECTRAL_COMPRESSION_GAMMA (0) */
```
For each window, the specturm is squashed down (compressed) logarithimcally using the formula:
COMPRESSED(spectrum) = log(1+gamma|spectrum|) / log(1+gamma)
Zero indicates no compression, and higher values of gamma have diminishing returns. I'm not sure it makes much difference in the onset detection, and setting it to 0 saves two expensive calls to log() per audio sample.


```c 
void      btt_set_oss_filter_cutoff              (BTT* self, double Hz);
double    btt_get_oss_filter_cutoff              (BTT* self);
/*default value: BTT_DEFAULT_OSS_FILTER_CUTOFF (10 Hz) */
```
The cutoff frequency of the low-pass filter applied to the spectral flux. You don't normally expect onsets more frequently than 10 or 15 Hz, so you might as well filter out everything above that. The filter order is an argument to btt_new(), and is constant for the life of the object.

```c 
void      btt_set_onset_threshold                (BTT* self, double num_std_devs);
double    btt_get_onset_threshold                (BTT* self);
/*default value: BTT_DEFAULT_ONSET_TREHSHOLD (1 standard deviation) */
```
Your onset callback will be called whenever the onset signal rises above the onset threshold. The threshold is adaptive. A windowed average of the onset signal is calculated, and the threshold remaines the given number of standard deviations above the mean. This works well for percussion music and poorly for everything else. In the future I might try to do something better. Note  poor functioning of the onset detection does not affect the tempo estimates and beat tracking.

