#include "../../BTT.h"

/*--------------------------------------------------------------------*/
void onset_detected_callback(void* SELF, unsigned long long sample_time)
{
  fprintf(stderr, "Yay, an onset was detected %llu samples into the audio stream!\r\n", sample_time);
}

/*--------------------------------------------------------------------*/
void beat_detected_callback (void* SELF, unsigned long long sample_time)
{
  fprintf(stderr, "Yay, a beat was detected %llu samples into the audio stream!\r\n", sample_time);
}

/*--------------------------------------------------------------------*/
int main(void)
{
  BTT* btt = btt_new_default();
  
  /* btt_new_default is the same as:
  BTT* btt = btt_new(BTT_SUGGESTED_SPECTRAL_FLUX_STFT_LEN,
                     BTT_SUGGESTED_SPECTRAL_FLUX_STFT_OVERLAP,
                     BTT_SUGGESTED_OSS_FILTER_ORDER,
                     BTT_SUGGESTED_OSS_LENGTH,
                     BTT_SUGGESTED_ONSET_THRESHOLD_N,
                     BTT_SUGGESTED_CBSS_LENGTH,
                     BTT_SUGGESTED_SAMPLE_RATE);
  */
  
  if(btt == NULL){perror("unable to create btt object"); exit(-2);}
  
  /* specify which functions should recieve notificaions */
  btt_set_onset_tracking_callback  (btt, onset_detected_callback, NULL);
  btt_set_beat_tracking_callback   (btt, beat_detected_callback , NULL);

  /*
   * it dosen't matter how big your buffers are,
   * it isn't related to the underlying analysis.
   */
  int buffer_size = 64;
  dft_sample_t buffer[buffer_size];
  
  for(;;)
  {
    /* Fill the buffer with your audio samples here.
     * The audio should be mono and the sample rate
     * by default is expected to be 44100 Hz.
     * You chan change it with the last
     * argument of btt_new, but the performance
     * of the algorithm may suffer.
     */
    btt_process(btt, buffer, buffer_size);
  }
}
