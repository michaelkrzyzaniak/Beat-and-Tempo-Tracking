# Beat-and-Tempo-Tracking
Beat-and-Tempo-Tracking taps its metaphorical foot along with the beat when it hears music. It is realtime and causal. It is an onset detector, tempo-estimator, and beat predictor. It is a combination of several state-of-the art methods. You feed buffers of audio data into it, and it notifies you when an onset occurs, when it has a new tempo estimate, or when it thinks the beat should happen. It is written in ANSI C and requires no external libraries or packages, and has no platform-dependent code. It was designed to run on ebmedded linux computers in musical robots, and It should run on anything.

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
