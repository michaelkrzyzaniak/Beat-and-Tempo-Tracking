#include "../../OBTAIN.h"
#include "MKAiff.h"
#include "Timestamp.h"

#define AUDIO_BUFFER_SIZE 64

void onset_detected_callback(void* SELF, unsigned long long sample_time);
void beat_detected_callback (void* SELF, unsigned long long sample_time);

/*--------------------------------------------------------------------*/
int main(void)
{
  MKAiff* aiff = aiffWithContentsOfFile("audio/beat.aiff");
  if(aiff == NULL){perror("unable to open audio/beat.aiff"); exit(-1);}
  aiffMakeMono(aiff);
  float secs = aiffDurationInSeconds(aiff);
  double sample_rate = aiffSampleRate(aiff);
  if(sample_rate != OBTAIN_SUGGESTED_SAMPLE_RATE)
    {fprintf(stderr, "Audio file should be %lf Hz but is %lf. Aborting.\r\n", (double)OBTAIN_SUGGESTED_SAMPLE_RATE, sample_rate); exit(-1);}

  Obtain* obtain = obtain_new(OBTAIN_SUGGESTED_SPECTRAL_FLUX_STFT_LEN,
                              OBTAIN_SUGGESTED_SPECTRAL_FLUX_STFT_OVERLAP,
                              OBTAIN_SUGGESTED_OSS_FILTER_ORDER,
                              OBTAIN_SUGGESTED_OSS_LENGTH,
                              OBTAIN_SUGGESTED_ONSET_THRESHOLD_N,
                              OBTAIN_SUGGESTED_CBSS_LENGTH,
                              sample_rate);
  if(obtain == NULL){perror("unable to create obtain object"); exit(-2);}
  
  MKAiff* onset_aiff = aiffWithDurationInSeconds(1, sample_rate, 16, secs+1);
  if(onset_aiff == NULL){perror("unable to create onset_aiff obejct"); exit(-1);}

  MKAiff* beat_aiff = aiffWithDurationInSeconds(1, sample_rate, 16, secs+1);
  if(beat_aiff == NULL){perror("unable to create beat_aiff obejct"); exit(-1);}

  aiffAppendSilenceInSeconds(onset_aiff, secs);
  aiffAppendSilenceInSeconds(beat_aiff , secs);

  obtain_set_onset_tracking_callback  (obtain, onset_detected_callback, onset_aiff);
  obtain_set_beat_tracking_callback   (obtain, beat_detected_callback , beat_aiff);
  
  dft_sample_t* buffer = calloc(AUDIO_BUFFER_SIZE, sizeof(*buffer));
  if(buffer == NULL){perror("unable to allocate buffer"); exit(-3);}
  
  timestamp_microsecs_t start = timestamp_get_current_time();
  for(;;)
    {
      int num_samples = aiffReadFloatingPointSamplesAtPlayhead(aiff, buffer, AUDIO_BUFFER_SIZE, aiffYes);
      obtain_process(obtain, buffer, num_samples);
      if(num_samples < AUDIO_BUFFER_SIZE)  break;
    }
  timestamp_microsecs_t end = timestamp_get_current_time();
  double process_time = (end-start)/(double)1000000;
  
  aiffSaveWithFilename(onset_aiff, "onsets.aiff");
  aiffSaveWithFilename(beat_aiff , "beats.aiff");
  
  fprintf(stderr, "Done. %f secs to process a %f sec audio file: %f%% realtime\r\n", process_time, secs, 100*(process_time/secs));
}

/*--------------------------------------------------------------------*/
/* store each onset as a single click in an AIFF file so we can compare it to the original */
void onset_detected_callback(void* SELF, unsigned long long sample_time)
{
  MKAiff* onset_aiff = (MKAiff*) SELF;
  aiffSetPlayheadToSamples     (onset_aiff, sample_time);
  float click = 1;
  aiffAddFloatingPointSamplesAtPlayhead(onset_aiff, &click, 1, aiffFloatSampleType, aiffYes);
}

/*--------------------------------------------------------------------*/
/* store each beat as a single click in an AIFF file so we can compare it to the original */
void beat_detected_callback (void* SELF, unsigned long long sample_time)
{
  MKAiff* beat_aiff = (MKAiff*) SELF;
  aiffSetPlayheadToSamples     (beat_aiff, sample_time);
  float click = 1;
  aiffAddFloatingPointSamplesAtPlayhead(beat_aiff, &click, 1, aiffFloatSampleType, aiffYes);
}
