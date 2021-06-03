//gcc *.c ../../src/*.c

//**************************************
//the suggested onset adjustment is 857
//the suggested beat adjustment is 1270
//**************************************
//**************************************
//the onset error is -0.913972
//the beat error is 50.123595
//**************************************

//**************************************
//the onset error mean is -0.913972
//the beat error mean is 50.546271
//the onset error std dev is 5.753604
//the beat error std dev is 280.149971 (6.4 millisecs)
//**************************************

#include "../../BTT.h"
#include "../../src/Statistics.h"

#include "MKAiff.h"

#include <math.h>

#define SAMPLE_RATE             44100
#define MIN_TEMPO               90  //inclusive
#define MAX_TEMPO               180 //exclusive
//#define MIN_TEMPO               82.6875  //inclusive once every 250 hops
//#define MAX_TEMPO               82.6875 //exclusive
//#define MIN_TEMPO               82
//#define MAX_TEMPO               82
#define NUM_TEMPO_SAMPLES       100
#define ANALYSIS_LENGTH_SECONDS 120

#define ANALYSIS_LENGTH_SAMPLES ((ANALYSIS_LENGTH_SECONDS) * (SAMPLE_RATE))

void onset_detected_callback(void* SELF, unsigned long long sample_time);
void beat_detected_callback (void* SELF, unsigned long long sample_time);

void get_analysis_latency_mean(double tempo, int onset_adjustment, int beat_adjustment, double* returned_onset_error, double* returned_beat_error);

OnlineAverage* compute_mean_error(MKAiff* reference, MKAiff* test, int* returned_max, int* returned_min);
int get_unit_impulse_locations_in_aiff(MKAiff* aiff, int* returned_locations, int returned_buffer_length);
int count_unit_impulses_in_aiff(MKAiff* aiff);

/*--------------------------------------------------------------------*/
int main(void)
{
  int i;
  
  OnlineAverage* onset_adjustment = online_average_new();
  OnlineAverage* beat_adjustment  = online_average_new();
  
  double onset_error, beat_error;
  
  if((onset_adjustment == NULL) || (beat_adjustment == NULL))
    {fprintf(stderr, "allocate OnlineAverage\r\n"); return -1;}
  
  for(i=0; i<NUM_TEMPO_SAMPLES; i++)
    {
      double tempo = MIN_TEMPO + i * (MAX_TEMPO - MIN_TEMPO) / (double)NUM_TEMPO_SAMPLES;
      get_analysis_latency_mean(tempo, 0/*BTT_DEFAULT_ANALYSIS_LATENCY_ADJUSTMENT*/, 0/*BTT_DEFAULT_ANALYSIS_LATENCY_BEAT_ADJUSTMENT*/, &onset_error, &beat_error);
      online_average_update(onset_adjustment, onset_error);
      online_average_update(beat_adjustment, beat_error);
    }

  int suggested_onset_adjustment = round(online_average_mean(onset_adjustment));
  int suggested_beat_adjustment  = round(online_average_mean(beat_adjustment));
  fprintf(stderr, "\r\n**************************************\r\n");
  fprintf(stderr, "the suggested onset adjustment is %i\r\n", suggested_onset_adjustment);
  fprintf(stderr, "the suggested beat adjustment is %i\r\n", suggested_beat_adjustment);
  fprintf(stderr, "**************************************\r\n\r\n");

  
  online_average_init(onset_adjustment);
  online_average_init(beat_adjustment);

  for(i=0; i<NUM_TEMPO_SAMPLES; i++)
    {
      double tempo = MIN_TEMPO + i * (MAX_TEMPO - MIN_TEMPO) / (double)NUM_TEMPO_SAMPLES;
      tempo += 0.5  * (MAX_TEMPO - MIN_TEMPO) / (double)NUM_TEMPO_SAMPLES;
      get_analysis_latency_mean(tempo, suggested_onset_adjustment, suggested_beat_adjustment, &onset_error, &beat_error);
      online_average_update(onset_adjustment, onset_error);
      online_average_update(beat_adjustment, beat_error);
    }

  fprintf(stderr, "\r\n**************************************\r\n");
  fprintf(stderr, "the onset error mean is %f\r\n", online_average_mean(onset_adjustment));
  fprintf(stderr, "the beat error mean is %f\r\n", online_average_mean(beat_adjustment));
  fprintf(stderr, "the onset error std dev is %f\r\n", online_average_std_dev(onset_adjustment));
  fprintf(stderr, "the beat error std dev is %f\r\n", online_average_std_dev(beat_adjustment));
  fprintf(stderr, "**************************************\r\n\r\n");
}

/*--------------------------------------------------------------------*/
void get_analysis_latency_mean(double tempo, int onset_adjustment, int beat_adjustment, double* returned_onset_error, double* returned_beat_error)
{
 //BTT* btt = btt_new_default();

  BTT* btt = btt_new(BTT_SUGGESTED_SPECTRAL_FLUX_STFT_LEN, BTT_SUGGESTED_SPECTRAL_FLUX_STFT_OVERLAP,
                     BTT_SUGGESTED_OSS_FILTER_ORDER      , BTT_SUGGESTED_OSS_LENGTH,
                     BTT_SUGGESTED_CBSS_LENGTH           , BTT_SUGGESTED_ONSET_THRESHOLD_N,
                     BTT_SUGGESTED_SAMPLE_RATE           , onset_adjustment, beat_adjustment);

  if(btt == NULL){perror("unable to create btt object"); exit(-2);}
 
  MKAiff* input_aiff = aiffWithDurationInSamples(1, SAMPLE_RATE, 16, ANALYSIS_LENGTH_SAMPLES);
  if(input_aiff == NULL){perror("unable to create input_aiff obejct"); exit(-1);}
 
  MKAiff* onset_aiff = aiffWithDurationInSamples(1, SAMPLE_RATE, 16, ANALYSIS_LENGTH_SAMPLES);
  if(onset_aiff == NULL){perror("unable to create onset_aiff obejct"); exit(-1);}

  MKAiff* beat_aiff = aiffWithDurationInSamples(1, SAMPLE_RATE, 16, ANALYSIS_LENGTH_SAMPLES);
  if(beat_aiff == NULL){perror("unable to create beat_aiff obejct"); exit(-1);}

  aiffAppendSilence(onset_aiff, ANALYSIS_LENGTH_SAMPLES);
  aiffAppendSilence(beat_aiff , ANALYSIS_LENGTH_SAMPLES);

  btt_set_onset_tracking_callback  (btt, onset_detected_callback, onset_aiff);
  btt_set_beat_tracking_callback   (btt, beat_detected_callback , beat_aiff);
  //btt_set_tracking_mode(btt, BTT_TEMPO_LOCKED_BEAT_TRACKING);
  btt_set_tracking_mode(btt, BTT_COUNT_IN_TRACKING);
  //btt_set_beat_prediction_adjustment(btt, 0);
  //btt_set_predicted_beat_trigger_index(btt, 30);
  
  int beat_num_samples = SAMPLE_RATE * 60.0 / tempo;
  
  dft_sample_t buffer = 0;
  
  unsigned  i;
  for(i=0; i<ANALYSIS_LENGTH_SAMPLES; i++)
    {
      buffer = (i%beat_num_samples == 0) ? 1 : 0;
      aiffAppendFloatingPointSamples(input_aiff, &buffer, 1, aiffFloatSampleType);
      btt_process(btt, &buffer, 1);
    }
 
 int onset_max_error, onset_min_error;
 int beat_max_error,  beat_min_error;
 
 OnlineAverage* onset_error = compute_mean_error(input_aiff, onset_aiff, &onset_max_error, &onset_min_error);
 OnlineAverage* beat_error  = compute_mean_error(input_aiff, beat_aiff , &beat_max_error , &beat_min_error );

 int num_input_beats = count_unit_impulses_in_aiff(input_aiff);
 
 fprintf(stderr, "Generated %i beats of metronome input at %i BPM over %i seconds of audio\r\n", num_input_beats, (int)tempo, (int)ANALYSIS_LENGTH_SECONDS);
 fprintf(stderr, "\tDetected %i onsets with error \r\n\t\tmean: %f\tmin: %i\tmax: %i\trange: %i\r\n", online_average_n(onset_error), online_average_mean(onset_error), onset_min_error, onset_max_error, onset_max_error-onset_min_error);
 fprintf(stderr, "\tDetected %i beats with error \r\n\t\tmean: %f\tmin: %i\tmax: %i\trange: %i\r\n", online_average_n(beat_error),  online_average_mean(beat_error),  beat_min_error,  beat_max_error,  beat_max_error-beat_min_error);
 
 *returned_onset_error = online_average_mean(onset_error);
 *returned_beat_error  = online_average_mean(beat_error);
 
  //aiffSaveWithFilename(input_aiff, "saved_output/1_input.aiff");
  //aiffSaveWithFilename(onset_aiff, "saved_output/2_onsets.aiff");
  //aiffSaveWithFilename(beat_aiff , "saved_output/3_beats.aiff");
  aiffDestroy(input_aiff);
  aiffDestroy(onset_aiff);
  aiffDestroy(beat_aiff);
  btt_destroy(btt);
  online_average_destroy(onset_error);
  online_average_destroy(beat_error);
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

/*--------------------------------------------------------------------*/
int count_unit_impulses_in_aiff(MKAiff* aiff)
{
  aiffRewindPlayheadToBeginning(aiff);
  int i, N = aiffDurationInSamples(aiff);
  int result = 0;
  float buffer;
  
  for(i=0; i<N; i++)
    {
      aiffReadFloatingPointSamplesAtPlayhead (aiff, &buffer, 1, aiffYes);
      if(buffer != 0)
        ++result;
    }
  return result;
}

/*--------------------------------------------------------------------*/
int get_unit_impulse_locations_in_aiff(MKAiff* aiff, int* returned_locations, int returned_buffer_length)
{
  aiffRewindPlayheadToBeginning(aiff);
  int i, N = aiffDurationInSamples(aiff);
  int result_n = 0;
  float buffer;
  
  for(i=0; i<N; i++)
    {
      aiffReadFloatingPointSamplesAtPlayhead (aiff, &buffer, 1, aiffYes);
      if(buffer != 0)
        if(result_n < returned_buffer_length)
          returned_locations[result_n++] = i;
    }
  return result_n;
}

/*--------------------------------------------------------------------*/
//caller frees result
OnlineAverage* compute_mean_error(MKAiff* reference, MKAiff* test, int* returned_max, int* returned_min)
{
  int reference_n = count_unit_impulses_in_aiff(reference);
  int test_n      = count_unit_impulses_in_aiff(reference);
  
  int* reference_impulse_locations = calloc(reference_n, sizeof(*reference_impulse_locations));
  int* test_impulse_locations      = calloc(reference_n, sizeof(*reference_impulse_locations));
  if((reference_impulse_locations == NULL) || (test_impulse_locations == NULL))
    {fprintf(stderr, "unable to calloc buffers for something\r\n"); return NULL;}
  
  reference_n = get_unit_impulse_locations_in_aiff(reference, reference_impulse_locations, reference_n);
  test_n = get_unit_impulse_locations_in_aiff(test, test_impulse_locations, test_n);
  
  OnlineAverage* result = online_average_new();
  if(result == NULL)
    {fprintf(stderr, "allocate OnlineAverage\r\n"); return NULL;}
  
  //assume impulses are in ascending order
  int i, j=0, below;
  int max=-1000000, min=1000000;
  for(i=0; i<test_n; i++)
    {
      for(; j<reference_n; j++)
        if(reference_impulse_locations[j] > test_impulse_locations[i])
          break;
    
      if(reference_impulse_locations[j] <= test_impulse_locations[i])
        continue;
    
      below = ((j-1)<0) ? 0 : j-1;
      int distance_above = reference_impulse_locations[j] - test_impulse_locations[i];
      int distance_below = test_impulse_locations[i] - reference_impulse_locations[below];
    
      double distance = (distance_below < distance_above) ? distance_below : -distance_above;

    
      if(distance > max) max = distance;
      if(distance < min) min = distance;
      online_average_update(result, distance);
    }
  *returned_max = max;
  *returned_min = min;
  return result;
}
